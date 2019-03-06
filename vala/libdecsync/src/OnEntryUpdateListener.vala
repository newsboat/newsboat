/**
 * libdecsync-vala - Subpath.vala
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

public interface OnEntryUpdateListener<T> : GLib.Object {
	public abstract bool matchesPath(Gee.List<string> path);
	public abstract void onEntriesUpdate(Gee.List<string> path, Gee.Collection<Decsync.Entry> entries, T extra);
}

public abstract class OnSubdirEntryUpdateListener<T> : GLib.Object, OnEntryUpdateListener<T> {

	public abstract Gee.List<string> subdir();
	public abstract void onSubdirEntryUpdate(Gee.List<string> path, Decsync.Entry entry, T extra);

	public bool matchesPath(Gee.List<string> path)
	{
		return path.size >= subdir().size && pathEquals(path.slice(0, subdir().size), subdir());
	}

	public void onEntriesUpdate(Gee.List<string> path, Gee.Collection<Decsync.Entry> entries, T extra)
	{
		foreach (var entry in entries) {
			onSubdirEntryUpdate(convertPath(path), entry, extra);
		}
	}

	private Gee.List<string> convertPath(Gee.List<string> path)
	{
		return path.slice(subdir().size, path.size);
	}
}

public abstract class OnSubfileEntryUpdateListener<T> : GLib.Object, OnEntryUpdateListener<T> {

	public abstract Gee.List<string> subfile();
	public abstract void onSubfileEntryUpdate(Decsync.Entry entry, T extra);

	public bool matchesPath(Gee.List<string> path)
	{
		return pathEquals(path, subfile());
	}

	public void onEntriesUpdate(Gee.List<string> path, Gee.Collection<Decsync.Entry> entries, T extra)
	{
		foreach (var entry in entries) {
			onSubfileEntryUpdate(entry, extra);
		}
	}
}

private bool pathEquals(Gee.List<string> path1, Gee.List<string> path2)
{
	if (path1.size != path2.size) {
		return false;
	}
	for (var i = 0; i < path1.size; ++i) {
		if (path1[i] != path2[i]) {
			return false;
		}
	}
	return true;
}
