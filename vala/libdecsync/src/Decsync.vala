/**
 * libdecsync-vala - Decsync.vala
 *
 * Copyright (C) 2018 Aldo Gunsing
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

public class Unit { public Unit() {} }

/**
 * The `DecSync` class represents an interface to synchronized key-value mappings stored on the file
 * system.
 *
 * The mappings can be synchronized by synchronizing the directory [dir]. The stored mappings are
 * stored in a conflict-free way. When the same keys are updated independently, the most recent
 * value is taken. This should not cause problems when the individual values contain as little
 * information as possible.
 *
 * Every entry consists of a path, a key and a value. The path is a list of strings which contains
 * the location to the used mapping. This can make interacting with the data easier. It is also used
 * to construct a path in the file system. All characters are allowed in the path. However, other
 * limitations of the file system may apply. For example, there may be a maximum length or the file
 * system may be case insensitive.
 *
 * To update an entry, use the method [setEntry]. When multiple keys in the same path are updated
 * simultaneous, it is encouraged to use the more efficient methods [setEntriesForPath] and
 * [setEntries].
 *
 * To get notified about updated entries, use the method [executeAllNewEntries] to get all updated
 * entries and execute the corresponding actions. The method [initObserver] creates a file observer
 * which is notified about the updated entries immediately.
 *
 * Sometimes, updates cannot be execute immediately. For example, if the name of a category is
 * updated when the category does not exist yet, the name cannot be changed. In such cases, the
 * updates have to be executed retroactively. In the example, the update can be executed when the
 * category is created. For such cases, use the method [executeStoredEntries].
 *
 * Finally, to initialize the stored entries to the most recent values, use the method
 * [initStoredEntries]. This method is almost exclusively used when the application is installed. It
 * is almost always followed by a call to [executeStoredEntries].
 *
 * @param T the type of the extra data passed to the [listeners] and [syncComplete].
 * @property dir the directory in which the synchronized DecSync files are stored.
 * For the default location, use [getDecsyncSubdir].
 * @property ownAppId the unique appId corresponding to the stored data by the application. There
 * must not be two simultaneous instances with the same appId. However, if an application is
 * reinstalled, it may reuse its old appId. In that case, it has to call [initStoredEntries] and
 * [executeStoredEntries]. Even if the old appId is not reused, it is still recommended call these.
 * For the default appId, use [getAppId].
 * @property listeners a list of listeners describing the actions to execute on every updated entry.
 * When an entry is updated, the method [OnEntryUpdateListener.onEntriesUpdate] is called on the
 * listener whose method [OnEntryUpdateListener.matchesPath] returns true.
 * @property syncComplete an optional function which is called when a sync is complete. For example,
 * it can be used to update the UI.
 */
public class Decsync<T> : GLib.Object {

	string dir;
	string ownAppId;
	string ownAppIdEncoded;
	Gee.Iterable<OnEntryUpdateListener<T>> listeners;
	DirectoryMonitor? monitor = null;

	/**
	 * Signal which is called when a sync is complete. For example, it can be used to update the UI.
	 */
	public signal void syncComplete(T extra);

	public Decsync(string dir, string ownAppId, Gee.Iterable<OnEntryUpdateListener<T>> listeners)
	{
		this.dir = dir;
		this.ownAppId = ownAppId;
		this.ownAppIdEncoded = FileUtils.urlencode(ownAppId);
		this.listeners = listeners;
	}

	/**
	 * Represents an [Entry] with its path.
	 */
	public class EntryWithPath {
		public Gee.List<string> path;
		public Entry entry;

		public EntryWithPath(string[] path, Entry entry)
		{
			this.path = toList(path);
			this.entry = entry;
		}

		public EntryWithPath.now(string[] path, Json.Node key, Json.Node value)
		{
			this.path = toList(path);
			this.entry = new Entry.now(key, value);
		}
	}

	/**
	 * Represents a key/value pair stored by DecSync. Additionally, it has a datetime property
	 * indicating the most recent update. It does not store its path, see [EntryWithPath].
	 */
	public class Entry {
		internal string datetime;
		public Json.Node key;
		public Json.Node value;

		public Entry(string datetime, Json.Node key, Json.Node value)
		{
			this.datetime = datetime;
			this.key = key;
			this.value = value;
		}

		public Entry.now(Json.Node key, Json.Node value)
		{
			this.datetime = new GLib.DateTime.now_utc().format("%FT%T");
			this.key = key;
			this.value = value;
		}

		internal string toLine()
		{
			var json = new Json.Node(Json.NodeType.ARRAY);
			var array = new Json.Array();
			array.add_string_element(this.datetime);
			array.add_element(this.key);
			array.add_element(this.value);
			json.set_array(array);
			return Json.to_string(json, false);
		}

		internal static Entry? fromLine(string line)
		{
			try {
				var json = Json.from_string(line);
				var array = json.get_array();
				if (array == null || array.get_length() != 3) {
					Log.w("Invalid entry " + line);
					return null;
				}
				var datetime = array.get_string_element(0);
				if (datetime == null) {
					Log.w("Invalid entry " + line);
					return null;
				}
				var key = array.get_element(1);
				var value = array.get_element(2);
				return new Entry(datetime, key, value);
			} catch (GLib.Error e) {
				Log.w("Invalid JSON: " + line + "\n" + e.message);
				return null;
			}
		}
	}

	private class EntriesLocation {
		public Gee.List<string> path;
		public File newEntriesFile;
		public File? storedEntriesFile;
		public File? readBytesFile;

		public EntriesLocation.getNewEntriesLocation(Decsync decsync, Gee.List<string> path, string appId)
		{
			var pathString = FileUtils.pathToString(path);
			var appIdEncoded = FileUtils.urlencode(appId);
			this.path = path;
			this.newEntriesFile = File.new_for_path(decsync.dir + "/new-entries/" + appIdEncoded + "/" + pathString);
			this.storedEntriesFile = File.new_for_path(decsync.dir + "/stored-entries/" + decsync.ownAppIdEncoded + "/" + pathString);
			this.readBytesFile = File.new_for_path(decsync.dir + "/read-bytes/" + decsync.ownAppIdEncoded + "/" + appIdEncoded + "/" + pathString);
		}

		public EntriesLocation.getStoredEntriesLocation(Decsync decsync, Gee.List<string> path)
		{
			var pathString = FileUtils.pathToString(path);
			this.path = path;
			this.newEntriesFile = File.new_for_path(decsync.dir + "/stored-entries/" + decsync.ownAppIdEncoded + "/" + pathString);
			this.storedEntriesFile = null;
			this.readBytesFile = null;
		}
	}

	/**
	 * Associates the given [value] with the given [key] in the map corresponding to the given
	 * [path]. This update is sent to synchronized devices.
	 */
	public void setEntry(string[] pathArray, Json.Node key, Json.Node value)
	{
		var entries = new Gee.ArrayList<Entry>();
		entries.add(new Entry.now(key, value));
		setEntriesForPath(toList(pathArray), entries);
	}

	/**
	 * Like [setEntry], but allows multiple entries to be set. This is more efficient if multiple
	 * entries share the same path.
	 *
	 * @param entriesWithPath entries with path which are inserted.
	 */
	public void setEntries(Gee.Collection<EntryWithPath> entriesWithPath)
	{
		var multiMap = groupByPath<EntryWithPath, Entry>(
			entriesWithPath,
			entryWithPath => { return entryWithPath.path; },
			entryWithPath => { return entryWithPath.entry; }
		);
		multiMap.get_keys().@foreach(path => {
			setEntriesForPath(path, multiMap.@get(path));
			return true;
		});
	}

	/**
	 * Like [setEntries], but only allows the entries to have the same path. Consequently, it can
	 * be slightly more convenient since the path has to be specified just once.
	 *
	 * @param path path to the map in which the entries are inserted.
	 * @param entries entries which are inserted.
	 */
	public void setEntriesForPath(Gee.List<string> path, Gee.Collection<Entry> entries)
	{
		Log.d("Write to path " + FileUtils.pathToString(path));
		var entriesLocation = new EntriesLocation.getNewEntriesLocation(this, path, ownAppId);

		// Write new entries
		var builder = new StringBuilder();
		foreach (var entry in entries) {
			builder.append(entry.toLine() + "\n");
		}
		try {
			FileUtils.writeFile(entriesLocation.newEntriesFile, builder.str, true);
		} catch (Error e) {
			Log.w(e.message);
		}

		// Update .decsync-sequence files
		while (!path.is_empty) {
			path.remove_at(path.size - 1);
			var dir = new EntriesLocation.getNewEntriesLocation(this, path, ownAppId).newEntriesFile;
			var file = dir.get_child(".decsync-sequence");

			// Get the old version
			int64 version = 0;
			if (file.query_exists()) {
				try {
					var stream = new DataInputStream(file.read());
					version = int64.parse(stream.read_line()); // Defaults to 0
				} catch (GLib.Error e) {
					Log.w(e.message);
				}
			}

			// Write the new version
			try {
				FileUtils.writeFile(file, (version + 1).to_string());
			} catch (Error e) {
				Log.w(e.message);
			}
		}

		// Update stored entries
		updateStoredEntries(entriesLocation, entries);
	}

	/**
	 * Initializes the monitor which watches the filesystem for updated entries and executes the
	 * corresponding actions.
	 *
	 * @param extra extra data passed to the [listeners].
	 */
	public void initMonitor(T extra)
	{
		try {
			var newEntriesDir = File.new_for_path(dir + "/new-entries");
			var parent = newEntriesDir.get_parent();
			if (!parent.query_exists()) {
				parent.make_directory_with_parents();
			}
			monitor = new DirectoryMonitor(newEntriesDir);
			monitor.changed.connect(pathString => {
				var pathEncoded = new Gee.ArrayList<string>.wrap(pathString.split("/"));
				pathEncoded.remove("");
				if (pathEncoded.is_empty || pathEncoded.last()[0] == '.') {
					return;
				}
				var path = new Gee.ArrayList<string>();
				path.add_all_iterator(pathEncoded.map<string>(part => { return FileUtils.urldecode(part); }));
				if (path.fold<bool>((part, seed) => { return part == null || seed; }, false)) {
					Log.w("Cannot decode path " + pathString);
					return;
				}
				var appId = path.first();
				path.remove_at(0);
				var entriesLocation = new EntriesLocation.getNewEntriesLocation(this, path, appId);
				if (appId != ownAppId && entriesLocation.newEntriesFile.query_file_type(FileQueryInfoFlags.NONE) == FileType.REGULAR) {
					executeEntriesLocation(entriesLocation, extra);
					Log.d("Sync complete");
					syncComplete(extra);
				}
			});
			Log.d("Initialized folder monitor for " + dir + "/new-entries");
		} catch (GLib.Error e) {
			Log.w(e.message);
		}
	}

	/**
	 * Gets all updated entries and executes the corresponding actions.
	 *
	 * @param extra extra data passed to the [listeners].
	 */
	public void executeAllNewEntries(T extra)
	{
		Log.d("Execute all new entries in " + dir);
		var newEntriesDir = File.new_for_path(dir + "/new-entries");
		var readBytesDir = File.new_for_path(dir + "/read-bytes/" + ownAppIdEncoded);
		Gee.Predicate<Gee.List<string>> pathPred = path => { return path.is_empty || path.first() != ownAppId; };
		FileUtils.listFilesRecursiveRelative(newEntriesDir, readBytesDir, pathPred)
			.map<EntriesLocation>(path => { return new EntriesLocation.getNewEntriesLocation(this, path.slice(1, path.size), path.first()); })
			.@foreach (entriesLocation => {
				executeEntriesLocation(entriesLocation, extra);
				return true;
			});
		Log.d("Sync complete");
		syncComplete(extra);
	}

	private void executeEntriesLocation(EntriesLocation entriesLocation, T extra, Gee.Predicate<Json.Node>? keyPred = null, Gee.Predicate<Json.Node>? valuePred = null)
	{
		// Get the number of read bytes
		int64 readBytes = 0;
		if (entriesLocation.readBytesFile != null && entriesLocation.readBytesFile.query_exists()) {
			try {
				var stream = new DataInputStream(entriesLocation.readBytesFile.read());
				readBytes = int64.parse(stream.read_line()); // Defaults to 0
			} catch (GLib.Error e) {
				Log.w(e.message);
			}
		}

		// Write the new number of read bytes (= size of the entry file)
		if (entriesLocation.readBytesFile != null) {
			try {
				var size = entriesLocation.newEntriesFile.query_info("standard::size", FileQueryInfoFlags.NONE).get_size();
				if (readBytes >= size) return;
				FileUtils.writeFile(entriesLocation.readBytesFile, size.to_string());
			} catch (GLib.Error e) {
				Log.w(e.message);
			}
		}

		Log.d("Execute entries of " + entriesLocation.newEntriesFile.get_path());

		// Execute the entries
		var entriesMap = new Gee.HashMap<Json.Node, Entry>(
			a => { return a.hash(); },
			(a, b) => { return a.equal(b); }
		);
		try {
			var stream = new DataInputStream(entriesLocation.newEntriesFile.read());
			stream.seek(readBytes, SeekType.SET);
			string line;
			while ((line = stream.read_line(null)) != null) {
				var entryLine = Entry.fromLine(line);
				if (entryLine == null) {
					continue;
				}
				if ((keyPred == null || keyPred(entryLine.key)) &&
						(valuePred == null || valuePred(entryLine.value))) {
					var key = entryLine.key;
					var entry = entriesMap.@get(key);
					if (entry == null || entryLine.datetime > entry.datetime) {
						entriesMap.@set(key, entryLine);
					}
				}
			}
		} catch (GLib.Error e) {
			Log.w(e.message);
		}
		var entries = new Gee.ArrayList<Entry>();
		entries.add_all(entriesMap.values);
		executeEntries(entriesLocation, entries, extra);
	}

	private void executeEntries(EntriesLocation entriesLocation, Gee.Collection<Entry> entries, T extra)
	{
		updateStoredEntries(entriesLocation, entries);

		var listener = getListener(entriesLocation.path);
		if (listener == null) {
			Log.e("Unknown action for path " + FileUtils.pathToString(entriesLocation.path));
			return;
		}

		listener.onEntriesUpdate(entriesLocation.path, entries, extra);
	}

	private void updateStoredEntries(EntriesLocation entriesLocation, Gee.Collection<Entry> entries)
	{
		if (entriesLocation.storedEntriesFile == null) {
			return;
		}

		try {
			var haveToFilterFile = false;
			if (entriesLocation.storedEntriesFile.query_exists()) {
				var stream = new DataInputStream(entriesLocation.storedEntriesFile.read());
				string line;
				while ((line = stream.read_line(null)) != null) {
					var entryLine = Entry.fromLine(line);
					if (entryLine == null) {
						continue;
					}
					var entriesIterator = entries.iterator();
					while (entriesIterator.has_next()) {
						entriesIterator.next();
						var entry = entriesIterator.get();
						if (entry.key.equal(entryLine.key)) {
							if (entry.datetime > entryLine.datetime) {
								haveToFilterFile = true;
							} else {
								entriesIterator.remove();
							}
						}
					}
				}
			}

			if (haveToFilterFile) {
				FileUtils.filterFile(entriesLocation.storedEntriesFile, line => {
					var entryLine = Entry.fromLine(line);
					if (entryLine == null) {
						return false;
					}
					return entries.fold<bool>((entry, seed) => { return !entry.key.equal(entryLine.key) && seed; }, true);
				});
			}

			var builder = new StringBuilder();
			entries.@foreach(entry => {
				builder.append(entry.toLine() + "\n");
				return true;
			});
			FileUtils.writeFile(entriesLocation.storedEntriesFile, builder.str, true);

			var maxDatetime = entries.fold<string?>((entry, seed) => { if (seed == null || entry.datetime > seed) return entry.datetime; else return seed; }, null);
			if (maxDatetime != null) {
				var latestStoredEntryFile = File.new_for_path(dir + "/info/" + ownAppIdEncoded + "/latest-stored-entry");
				string? latestDatetime = null;
				try {
					var stream = new DataInputStream(latestStoredEntryFile.read());
					latestDatetime = stream.read_line();
				} catch (GLib.Error e) {
					Log.w(e.message);
				}
				if (latestDatetime == null || maxDatetime > latestDatetime) {
					FileUtils.writeFile(latestStoredEntryFile, maxDatetime);
				}
			}
		}
		catch (GLib.Error e)
		{
			Log.w(e.message);
		}
	}

	/**
	 * Gets all stored entries satisfying the predicates and executes the corresponding actions.
	 *
	 * @param executePath path to the entries to executes. This can be either a file or a directory.
	 * If it specifies a file, the entries in that file are executed. If it specifies a directory,
	 * all entries in all subfiles are executed.
	 * @param extra extra data passed to the [listeners].
	 * @param keyPred optional predicate on the keys. The key has to satisfy this predicate to be
	 * executed.
	 * @param valuePred optional predicate on the values. The value has to satisfy this predicate to
	 * be executed.
	 * @param pathPred optional predicate on the subpaths. Each subpath has to satisfy this
	 * predicate to be executed. This holds for directories as well. Furthermore, the path of
	 * specified in [executePath] is not part of the argument.
	 */
	public void executeStoredEntries(string[] executePathArray, T extra,
		Gee.Predicate<Json.Node>? keyPred = null,
		Gee.Predicate<Json.Node>? valuePred = null,
		Gee.Predicate<Gee.List<string>>? pathPred = null)
	{
		var executePath = toList(executePathArray);
		var executePathString = FileUtils.pathToString(executePath);
		var executeDir = File.new_for_path(dir + "/stored-entries/" + ownAppIdEncoded + "/" + executePathString);
		FileUtils.listFilesRecursiveRelative(executeDir, null, pathPred)
			.@foreach(path => {
				path.insert_all(0, executePath);
				var entriesLocation = new EntriesLocation.getStoredEntriesLocation(this, path);
				executeEntriesLocation(entriesLocation, extra, keyPred, valuePred);
				return true;
			});
	}

	/**
	 * Initializes the stored entries. This method does not execute any actions. This is often
	 * followed with a call to [executeStoredEntries].
	 */
	public void initStoredEntries()
	{
		// Get the most up-to-date appId
		var appId = latestAppId();

		// Copy the stored files and update the read bytes
		if (appId != ownAppId) {
			var appIdEncoded = FileUtils.urlencode(appId);

			try {
				FileUtils.@delete(File.new_for_path(dir + "/stored-entries/" + ownAppIdEncoded));
				FileUtils.copy(File.new_for_path(dir + "/stored-entries/" + appIdEncoded), File.new_for_path(dir + "/stored-entries/" + ownAppIdEncoded));
			} catch (GLib.Error e) {
				Log.w(e.message);
			}

			try {
				FileUtils.@delete(File.new_for_path(dir + "/read-bytes/" + ownAppIdEncoded));
				FileUtils.copy(File.new_for_path(dir + "/read-bytes/" + appIdEncoded), File.new_for_path(dir + "/read-bytes/" + ownAppIdEncoded));
			} catch (GLib.Error e) {
				Log.w(e.message);
			}
			var newEntriesDir = File.new_for_path(dir + "/new-entries/" + appIdEncoded);
			var ownReadBytesDir = File.new_for_path(dir + "/read-bytes/" + ownAppIdEncoded + "/" + appIdEncoded);
			FileUtils.listFilesRecursiveRelative(newEntriesDir, ownReadBytesDir).@foreach(path => {
				var pathString = FileUtils.pathToString(path);
				try {
					var newEntriesFile = File.new_for_path(dir + "/new-entries/" + appIdEncoded + "/" + pathString);
					var size = newEntriesFile.query_info("standard::size", FileQueryInfoFlags.NONE).get_size();
					var readBytesFile = File.new_for_path(dir + "/read-bytes/" + ownAppIdEncoded + "/" + appIdEncoded + "/" + pathString);
					FileUtils.writeFile(readBytesFile, size.to_string());
				} catch (GLib.Error e) {
					Log.w(e.message);
				}
				return true;
			});
		}
    }

	/**
	 * Returns the most up-to-date appId. This is the appId which has stored the most recent entry.
	 * In case of a tie, the appId corresponding to the current application is used, if possible.
	 */
	public string latestAppId()
	{
		string? latestAppId = null;
		string? latestDatetime = null;
		var infoDir = File.new_for_path(dir + "/info");
		try {
			var enumerator = infoDir.enumerate_children("standard::*", FileQueryInfoFlags.NONE);
			FileInfo info;
			while ((info = enumerator.next_file(null)) != null) {
				if (info.get_name()[0] == '.') {
					continue;
				}

				var appId = FileUtils.urldecode(info.get_name());
				var file = File.new_for_path(dir + "/info/" + info.get_name() + "/latest-stored-entry");

				if (appId == null ||
					!file.query_exists() ||
					file.query_file_type(FileQueryInfoFlags.NONE) != FileType.REGULAR)
				{
					continue;
				}

				string? datetime = null;
				try {
					var stream = new DataInputStream(file.read());
					datetime = stream.read_line();
				} catch (GLib.Error e) {
					Log.w(e.message);
				}
				if (datetime > latestDatetime ||
					appId == ownAppId && datetime == latestDatetime)
				{
					latestDatetime = datetime;
					latestAppId = appId;
				}
			}
		} catch (GLib.Error e) {
			Log.w(e.message);
		}

		return latestAppId ?? ownAppId;
	}

	/**
	 * Returns the value of the given [key] in the map of the given [path], and in the given
	 * [DecSync directory][decsyncDir] without specifying an appId, or `null` if there is no
	 * such value. The use of this method is discouraged. It is recommended to use the method
	 * [executeStoredEntries] when possible.
	 */
	public static Json.Node? getStoredStaticValue(string decsyncDir, string[] pathArray, Json.Node key)
	{
		Log.d("Get value for key " + Json.to_string(key, false) + " for path " + string.joinv("/", pathArray) + " in " + decsyncDir);
		var path = toList(pathArray);
		var pathString = FileUtils.pathToString(path);
		Json.Node? result = null;
		string? maxDatetime = null;
		var storedEntriesDir = File.new_for_path(decsyncDir + "/stored-entries");
		try {
			var enumerator = storedEntriesDir.enumerate_children("standard::*", FileQueryInfoFlags.NONE);
			FileInfo info;
			while ((info = enumerator.next_file(null)) != null) {
				if (info.get_name()[0] == '.') {
					continue;
				}

				var appIdEncoded = info.get_name();
				var file = File.new_for_path(decsyncDir + "/stored-entries/" + appIdEncoded + "/" + pathString);
				if (!file.query_exists() || file.query_file_type(FileQueryInfoFlags.NONE) != FileType.REGULAR) {
					continue;
				}

				var stream = new DataInputStream(file.read());
				string line;
				while ((line = stream.read_line(null)) != null) {
					var entry = Entry.fromLine(line);
					if (entry == null) {
						continue;
					}
					if (entry.key.equal(key) && (maxDatetime == null || entry.datetime > maxDatetime)) {
						maxDatetime = entry.datetime;
						result = entry.value;
					}
				}
			}
		} catch (GLib.Error e) {
			Log.w(e.message);
		}

		return result;
	}

	private OnEntryUpdateListener<T>? getListener(Gee.List<string> path)
	{
		foreach (var listener in listeners) {
			if (listener.matchesPath(path)) {
				return listener;
			}
		}
		return null;
	}
}

/**
 * Returns the path to the DecSync subdirectory in a [decsyncBaseDir] for a [syncType] and
 * optionally with a [collection].
 *
 * @param decsyncBaseDir the path to the main DecSync directory, or null for the default one.
 * @param syncType the type of data to sync. For example, "rss", "contacts" or "calendars".
 * @param collection an optional collection identifier when multiple instances of the [syncType] are
 * supported. For example, this is the case for "contacts" and "calendars", but not for "rss".
 */
public string getDecsyncSubdir(string? decsyncBaseDir, string syncType, string? collection = null)
{
	string dir = decsyncBaseDir ?? getDefaultDecsyncBaseDir();
	dir += "/" + FileUtils.urlencode(syncType);
	if (collection != null) {
		dir += "/" + FileUtils.urlencode(collection);
	}
	return dir;
}

/**
 * Returns the default DecSync directory. This is the "decsync" subdirectory on the user data dir
 * ("~/.local/share" by default).
 */
public string getDefaultDecsyncBaseDir()
{
	return GLib.Environment.get_user_data_dir() + "/decsync";
}

/**
 * Returns a list of DecSync collections inside a [decsyncBaseDir] for a [syncType]. This function
 * does not apply for sync types with single instances.
 *
 * @param decsyncBaseDir the path to the main DecSync directory, or null for the default one.
 * @param syncType the type of data to sync. For example, "contacts" or "calendars".
 * @param ignoreDeleted `true` to ignore deleted collections. A collection is considered deleted if
 * the most recent value of the key "deleted" with the path ["info"] is set to `true`.
 */
public Gee.ArrayList<string> listDecsyncCollections(string? decsyncBaseDir, string syncType, bool ignoreDeleted = true) throws GLib.Error
{
	var decsyncSubdir = File.new_for_path(getDecsyncSubdir(decsyncBaseDir, syncType));
	var enumerator = decsyncSubdir.enumerate_children("standard::*", FileQueryInfoFlags.NONE);
	FileInfo info;
	Gee.ArrayList<string> result = new Gee.ArrayList<string>();
	while ((info = enumerator.next_file(null)) != null) {
		if (info.get_file_type() != FileType.DIRECTORY || info.get_name()[0] == '.') {
			continue;
		}
		if (ignoreDeleted) {
			var deleted = Decsync.getStoredStaticValue(decsyncSubdir.get_child(info.get_name()).get_path(), {"info"}, stringToNode("deleted"));
			if (deleted != null && deleted.get_boolean()) {
				continue;
			}
		}
		var collection = FileUtils.urldecode(info.get_name());
		if (collection != null) {
			result.add(collection);
		}
	}
	return result;
}

/**
 * Returns the appId of the current device and application combination.
 *
 * @param appName the name of the application.
 * @param id an optional integer (between 0 and 100000 exclusive) to distinguish different instances
 * on the same device and application.
 */
public string getAppId(string appName, int? id = null)
{
	string appId = GLib.Environment.get_host_name() + "-" + appName;
	if (id == null) {
		return appId;
	} else {
		return appId + "-" + "%05d".printf(id);
	}
}
