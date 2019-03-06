/**
 * libdecsync-vala - FileUtils.vala
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

public class FileUtils : GLib.Object {

	public static void writeFile(File file, string content, bool append = false) throws GLib.Error
	{
		var parent = file.get_parent();
		if (!parent.query_exists()) {
			parent.make_directory_with_parents();
		}

		GLib.FileOutputStream stream;
		if (append) {
			stream = file.append_to(FileCreateFlags.NONE);
		} else {
			if (file.query_exists()) {
				file.@delete();
			}
			stream = file.create(FileCreateFlags.REPLACE_DESTINATION);
		}
		stream.write(content.data);
	}

	public static void @delete(File src) throws GLib.Error
	{
		if (!src.query_exists()) {
			return;
		}
		if (src.query_file_type(FileQueryInfoFlags.NONE) == FileType.DIRECTORY) {
			var enumerator = src.enumerate_children("standard::name", FileQueryInfoFlags.NONE);
			FileInfo info;
			while ((info = enumerator.next_file(null)) != null) {
				var name = info.get_name();
				@delete(src.get_child(name));
			}
		}
		src.@delete();
	}

	public static void copy(File src, File dst, bool overwrite = false) throws GLib.Error
	{
		switch (src.query_file_type(FileQueryInfoFlags.NONE)) {
			case FileType.REGULAR:
				var parent = dst.get_parent();
				if (!parent.query_exists()) {
					parent.make_directory_with_parents();
				}
				src.copy(dst, overwrite ? FileCopyFlags.OVERWRITE : FileCopyFlags.NONE);
				return;
			case FileType.DIRECTORY:
				dst.make_directory_with_parents();
				var enumerator = src.enumerate_children("standard::name", FileQueryInfoFlags.NONE);
				FileInfo info;
				while ((info = enumerator.next_file(null)) != null) {
					var name = info.get_name();
					copy(src.get_child(name), dst.get_child(name), overwrite);
				}
				return;
		}
	}

	public static void filterFile(File file, Gee.Predicate<string> linePred) throws GLib.Error
	{
		var tempFile = File.new_for_path(file.get_parent().get_path() + "." + file.get_basename() + ".tmp");
		var instream = new DataInputStream(file.read());
		var outstream = new DataOutputStream(tempFile.create(FileCreateFlags.NONE));
		string line;
		while ((line = instream.read_line(null)) != null) {
			if (linePred(line)) {
				outstream.put_string(line + "\n");
			}
		}
		tempFile.move(file, FileCopyFlags.OVERWRITE);
	}

	public static Gee.ArrayList<Gee.ArrayList<string>> listFilesRecursiveRelative(File src, File? readBytesSrc = null, Gee.Predicate<Gee.List<string>>? pathPred = null)
	{
		if (src.get_basename()[0] == '.') {
			return new Gee.ArrayList<Gee.ArrayList<string>>();
		}
		if (pathPred != null && !pathPred(new Gee.ArrayList<string>())) {
			return new Gee.ArrayList<Gee.ArrayList<string>>();
		}

		switch (src.query_file_type(FileQueryInfoFlags.NONE)) {
			case FileType.REGULAR:
				var result = new Gee.ArrayList<Gee.ArrayList<string>>();
				result.add(new Gee.ArrayList<string>());
				return result;
			case FileType.DIRECTORY:
				// Skip same versions
				if (readBytesSrc != null) {
					var file = src.get_child(".decsync-sequence");
					string? version = null;
					if (file.query_exists()) {
						try {
							version = new DataInputStream(file.read()).read_line();
						} catch (GLib.Error e) {
							Log.w(e.message);
						}
					}
					var readBytesFile = readBytesSrc.get_child(".decsync-sequence");
					string? readBytesVersion = null;
					if (readBytesFile.query_exists()) {
						try {
							readBytesVersion = new DataInputStream(readBytesFile.read()).read_line();
						} catch (GLib.Error e) {
							Log.w(e.message);
						}
					}
					if (version != null) {
						if (version == readBytesVersion) {
							return new Gee.ArrayList<Gee.ArrayList<string>>();
						} else {
							try {
								copy(file, readBytesFile, true);
							} catch (GLib.Error e) {
								Log.w(e.message);
							}
						}
					}
				}

				var result = new Gee.ArrayList<Gee.ArrayList<string>>();
				try {
					var enumerator = src.enumerate_children("standard::name", FileQueryInfoFlags.NONE);
					FileInfo info;
					while ((info = enumerator.next_file(null)) != null) {
						string name = info.get_name();
						string? nameDecoded = urldecode(name);
						if (nameDecoded == null) {
							Log.w("Cannot decode name " + name);
							continue;
						}

						var newReadBytesSrc = readBytesSrc == null ? null : readBytesSrc.get_child(name);
						Gee.Predicate<Gee.List<string>>? newPathPred = null;
						if (pathPred != null) {
							newPathPred = path => { path.insert(0, nameDecoded); return pathPred(path); };
						}
						var paths = listFilesRecursiveRelative(src.get_child(name), newReadBytesSrc, newPathPred);
						foreach (var path in paths) {
							path.insert(0, nameDecoded);
						}
						result.add_all(paths);
					}
				} catch (GLib.Error e) {
					Log.w(e.message);
				}
				return result;
			default:
				return new Gee.ArrayList<Gee.ArrayList<string>>();
		}
	}

    public static string pathToString(Gee.List<string> path)
    {
        var encodedPath = new Gee.ArrayList<string>();
        encodedPath.add_all_iterator(path.map<string>(part => { return urlencode(part); }));
        return string.joinv("/", encodedPath.to_array());
    }

    public static string urlencode(string input)
    {
	    var builder = new StringBuilder();
	    for (int i = 0; i < input.length; i++) {
		    char byte = input[i];
		    if (byte.isalnum() || "-_.~".contains(byte.to_string())) {
			    builder.append_c(byte);
		    } else {
			    builder.append("%%%2X".printf(byte));
		    }
	    }
	    var output = builder.str;

	    if (output != "" && output[0] == '.') {
		    output = "%2E" + output.substring(1);
	    }

	    return output;
    }

    public static string? urldecode(string input)
    {
	    var builder = new StringBuilder();
	    for (int i = 0; i < input.length; i++) {
		    char byte = input[i];
		    if (byte != '%') {
			    builder.append_c(byte);
		    } else {
			    if (i + 2 >= input.length) return null;
			    if (!input[i+1].isxdigit() || !input[i+2].isxdigit()) return null;
			    char value1 = (char)input[i+1].xdigit_value();
			    char value2 = (char)input[i+2].xdigit_value();
			    builder.append_c(16 * value1 + value2);
			    i += 2;
		    }
	    }
	    return builder.str;
    }
}
