/**
 * libdecsync-vala - Log.vala
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

public class Log : GLib.Object {
	const string TAG = "DecSync";

	private static void log(LogLevelFlags level, string message)
	{
		GLib.log_structured(TAG, level, "MESSAGE", "%s", message);
	}

	public static void e(string message)
	{
		log(GLib.LogLevelFlags.LEVEL_CRITICAL, message);
	}

	public static void w(string message)
	{
		log(GLib.LogLevelFlags.LEVEL_WARNING, message);
	}

	public static void i(string message)
	{
		log(GLib.LogLevelFlags.LEVEL_INFO, message);
	}

	public static void d(string message)
	{
		log(GLib.LogLevelFlags.LEVEL_DEBUG, message);
	}
}
