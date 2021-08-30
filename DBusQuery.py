#!/usr/bin/python3
#
#	DBusQuery - Shallow DBus abstration layer for common tasks.
#	Copyright (C) 2021-2021 Johannes Bauer
#
#	This file is part of pycommon.
#
#	pycommon is free software; you can redistribute it and/or modify
#	it under the terms of the GNU General Public License as published by
#	the Free Software Foundation; this program is ONLY licensed under
#	version 3 of the License, later versions are explicitly excluded.
#
#	pycommon is distributed in the hope that it will be useful,
#	but WITHOUT ANY WARRANTY; without even the implied warranty of
#	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#	GNU General Public License for more details.
#
#	You should have received a copy of the GNU General Public License
#	along with pycommon; if not, write to the Free Software
#	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
#	Johannes Bauer <JohannesBauer@gmx.de>
#
#	File UUID 6b03ea1d-0e93-4901-9c8c-4db5fdbd41d6

import time
import subprocess

class DBusQuery():
	@classmethod
	def is_screensaver_active(cls):
		output = subprocess.check_output([ "dbus-send", "--print-reply=literal", "--dest=org.mate.ScreenSaver", "/org/mate/ScreenSaver", "org.mate.ScreenSaver.GetActive" ])
		return b"boolean true" in output

	@classmethod
	def lock_screensaver(cls):
		subprocess.check_call([ "dbus-send", "--type=method_call", "--dest=org.mate.ScreenSaver", "/org/mate/ScreenSaver", "org.mate.ScreenSaver.Lock" ])

	@classmethod
	def suspend_to_ram(cls):
		subprocess.check_call([ "dbus-send", "--system", "--print-reply", "--dest=org.freedesktop.login1", "/org/freedesktop/login1", "org.freedesktop.login1.Manager.Suspend", "boolean:true" ])

	@classmethod
	def lock_screensaver_and_suspend_to_ram(cls):
		cls.lock_screensaver()
		while not cls.is_screensaver_active():
			time.sleep(0.3)
		cls.suspend_to_ram()

if __name__ == "__main__":
	import time
	DBusQuery.lock_screensaver_and_suspend_to_ram()
#	DBusQuery.suspend_to_ram()
#	DBusQuery.lock_screensaver()

	for i in range(100):
		print(DBusQuery.is_screensaver_active())
		time.sleep(0.3)


