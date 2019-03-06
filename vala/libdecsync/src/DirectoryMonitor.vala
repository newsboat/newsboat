/**
 * libdecsync-vala - DirectoryMonitor.vala
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

public class DirectoryMonitor : GLib.Object {

	private File mDir;
	private string mPath;
	private FileMonitor mMonitor;
	private Gee.ArrayList<DirectoryMonitor> mChilds = new Gee.ArrayList<DirectoryMonitor>();

	public signal void changed(string path);

	public DirectoryMonitor(File dir) throws GLib.Error
	{
		this.withPath(dir, "");
	}

	private DirectoryMonitor.withPath(File dir, string path) throws GLib.Error
	{
		mDir = dir;
		mPath = path;
		var currentDir = File.new_for_path(dir.get_path() + path);
		mMonitor = currentDir.monitor_directory(FileMonitorFlags.NONE);
		mMonitor.changed.connect((file, otherFile, event) => {
			if (file.get_path() != mDir.get_path() + path) {
				onEvent(path + "/" + file.get_basename(), event);
			}
		});
		Log.d("Monitor created for " + currentDir.get_path() + " (folder " + dir.get_path() + ")");

		var enumerator = currentDir.enumerate_children("standard::*", FileQueryInfoFlags.NONE);
		FileInfo info = null;
		while (((info = enumerator.next_file(null)) != null)) {
			if (info.get_file_type() == FileType.DIRECTORY) {
				var childMonitor = new DirectoryMonitor.withPath(mDir, path + "/" + info.get_name());
				childMonitor.changed.connect((path) => {
					changed(path);
				});
				mChilds.add(childMonitor);
			}
		}
	}

	private void onEvent(string path, FileMonitorEvent event)
	{
	    Log.d("Received inotify event " + event.to_string() + " at " + mDir.get_path() + "/" + path);
		switch (event) {
			case FileMonitorEvent.DELETED:
				foreach (var c in mChilds) {
					if (c.mPath == path) {
						mChilds.remove(c);
						break;
					}
				}
				break;
			case FileMonitorEvent.CREATED:
			case FileMonitorEvent.CHANGED:
				var file = File.new_for_path(mDir.get_path() + path);
				if (file.query_file_type(FileQueryInfoFlags.NONE) == FileType.DIRECTORY) {
					try {
						var childMonitor = new DirectoryMonitor.withPath(mDir, path);
						childMonitor.changed.connect((path) => {
							changed(path);
						});
						mChilds.add(childMonitor);
					} catch (GLib.Error e) {
						Log.w(e.message);
					}
				} else {
					changed(path);
				}
			break;
		}
	}
}
