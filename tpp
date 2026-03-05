#!/usr/bin/env python3
#	jbin - Joe's miscellaneous scripts, tools and configs
#	tpp: Test Python Project wrapper for linting, coverage and testing of Python code
#	Copyright (C) 2026-2026 Johannes Bauer
#
#	This file is part of jbin.
#
#	jbin is free software; you can redistribute it and/or modify
#	it under the terms of the GNU General Public License as published by
#	the Free Software Foundation; this program is ONLY licensed under
#	version 3 of the License, later versions are explicitly excluded.
#
#	jbin is distributed in the hope that it will be useful,
#	but WITHOUT ANY WARRANTY; without even the implied warranty of
#	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#	GNU General Public License for more details.
#
#	You should have received a copy of the GNU General Public License
#	along with jbin. If not, see <http://www.gnu.org/licenses/>.
#
#	Johannes Bauer <JohannesBauer@gmx.de>

import os
import sys
import configparser
import contextlib
import json
import unittest
import subprocess
import coverage
from FriendlyArgumentParser import FriendlyArgumentParser

class CustomTestResult(unittest.TextTestResult):
	def __init__(self, *args, **kwargs):
		super().__init__(*args, **kwargs)
		self._failure_details = [ ]

	@property
	def failure_details(self):
		return self._failure_details

	def addFailure(self, test, error):
		super().addFailure(test, error)
		self._failure_details.append(test)

	def addError(self, test, error):
		super().addError(test, error)
		self._failure_details.append(test)

class CustomTestRunner(unittest.TextTestRunner):
	resultclass = CustomTestResult

class TPP():
	def __init__(self, args):
		self._args = args
		self._config = configparser.ConfigParser(allow_no_value = True, delimiters = ("=", ), allow_unnamed_section =  True)
		self._config.optionxform = str
		self._config.read([ self._args.config_filename ])
		self._enabled_linter_patterns = [ self._parse_linter_pattern(pattern_str) for pattern_str in self._section("linter.enable") ]
		self._disabled_linter_patterns = [ self._parse_linter_pattern(pattern_str) for pattern_str in self._section("linter.disable") ]
		self._returncode = 127

	@property
	def default_section(self):
		return self._config[configparser.UNNAMED_SECTION]

	@property
	def statefile(self):
		return os.path.dirname(self._args.config_filename) + ".tpp_state.json"

	@property
	def returncode(self):
		return self._returncode

	def _section(self, section_name: str):
		if self._config.has_section(section_name):
			return self._config[section_name]
		else:
			return { }

	def _matches_linter_pattern(self, finding: dict, pattern: dict):
		doesmatch = all(finding.get(key) == value for (key, value) in pattern.items())
		return doesmatch

	def _matches_any_linter_pattern(self, finding: dict, patterns: list[dict]):
		return any(self._matches_linter_pattern(finding, pattern) for pattern in patterns)

	def _parse_linter_pattern(self, pattern_str: str):
		pattern_dict = { }
		(module, obj, symbol) = pattern_str.split(":")
		if module != "*":
			pattern_dict["module"] = module
		if obj != "*":
			pattern_dict["obj"] = obj
		if symbol != "*":
			pattern_dict["symbol"] = symbol
		return pattern_dict

	def _finding_valid(self, finding: dict):
		# Check if finding is ignored
		finding["idstr"] = f"{finding['module']}:{finding['obj']}:{finding['symbol']}"
		if self._matches_any_linter_pattern(finding, self._disabled_linter_patterns):
			return False
		if self._matches_any_linter_pattern(finding, self._enabled_linter_patterns):
			return True
		return finding["type"] in [ "error", "warning" ]

	def _run_linter(self):
		proc = subprocess.run([ "pylint", "--indent-string", "\\t", "--ignore-long-lines", ".*", "-f", "json", self.default_section["lint_module"] ], check = False, capture_output = True)
		lint_result = json.loads(proc.stdout)
		findings = [ finding for finding in lint_result if self._finding_valid(finding) ]
		if len(findings) == 0:
			print("No linter findings.")
		else:
			self._returncode |= 2
			print(f"{len(findings)} linter findings:")
		for finding in findings:
			print(f"{finding['idstr']} {finding['path']} line {finding['line']} {finding['message']}")

		if (len(findings) > 0) and self._args.vim:
			vim_fixfile = "/tmp/vim_linter_fixes.txt"
			with open(vim_fixfile, "w") as f:
				for finding in findings:
					print(f"{finding['path']}:{finding['line']}:{finding['message']}", file = f)
			subprocess.run([ "vi", "-q", vim_fixfile ])

	@contextlib.contextmanager
	def _coverage(self):
		if self._args.coverage or self._args.coverage_scripts:
			self._cov = coverage.Coverage(omit = [ "tpp" ])
			self._cov.start()
		try:
			yield
		finally:
			if self._args.coverage or self._args.coverage_scripts:
				self._cov.stop()
				self._cov.save()

	def _get_testcases_from_suite(self, suite: unittest.TestSuite):
		for obj in suite:
			if isinstance(obj, unittest.TestSuite):
				yield from self._get_testcases_from_suite(obj)
			else:
				yield obj

	def _filter_testcases(self, suite: unittest.TestSuite, test_judgement: "Callable"):
		result = unittest.TestSuite()
		for test in self._get_testcases_from_suite(suite):
			if test_judgement(test):
				result.addTest(test)
		return result

	def _filter_testcases_id_endswith(self, suite: unittest.TestSuite, permissible_id_suffixes: set[str]):
		return self._filter_testcases(suite, lambda test: any(test.id().endswith(suffix) for suffix in permissible_id_suffixes))

	def _filter_testcases_only_permissible(self, suite: unittest.TestSuite, permissible_ids: set[str]):
		return self._filter_testcases(suite, lambda test: test.id() in permissible_ids)

	def _run_tests(self):
		if self._args.set_test_environment:
			for (key, value) in self._section("test.env_vars").items():
				os.environ[key] = value

		runner = CustomTestRunner(durations = 0 if self._args.show_test_duration else None)
		with self._coverage():
			# Load all tests first, *inside* coverage window (some code is
			# already executed at loading time and we want to catch that)
			suite = unittest.defaultTestLoader.loadTestsFromName(self.default_section["test_module"])

			# If we have explicit command line, filter those
			if len(self._args.testcase) > 0:
				print("Limiting testcases only to those given on command line.", file = sys.stderr)
				suite = self._filter_testcases_id_endswith(suite, self._args.testcase)
			else:
				# Do we have failed testcases in the state file?
				if (not self._args.test_all) and os.path.isfile(self.statefile):
					print("Limiting testcases only to those recorded in state file.", file = sys.stderr)
					with open(self.statefile) as f:
						statefile = json.load(f)
					suite = self._filter_testcases_only_permissible(suite, statefile)
			result = runner.run(suite)

		if len(result.failure_details) == 0:
			# No failed testcases. Delete statefile
			with contextlib.suppress(FileNotFoundError):
				os.unlink(self.statefile)
		else:
			# Something failed, record it
			self._returncode |= 1
			if self._args.limit_failed_recording == 0:
				record_tests = result.failure_details
			else:
				record_tests = result.failure_details[:self._args.limit_failed_recording]
			with open(self.statefile, "w") as f:
				f.write(json.dumps([ test.id() for test in record_tests ]))

	def _coverage_scripts(self):
		for script_name in self._section("coverage.scripts"):
			subprocess.run([ script_name ])

	def _coverage_report(self):
		# Load coverage data again because the scripts might have appended data
		self._cov = coverage.Coverage()
		self._cov.load()
		self._cov.report(omit = list(self._section("coverage.ignore")))
		self._cov.html_report(omit = list(self._section("coverage.ignore")))

	def _clean(self):
		with contextlib.suppress(FileNotFoundError):
			os.unlink(".coverage")
		with contextlib.suppress(FileNotFoundError):
			os.unlink(self.statefile)

	def run(self):
		self._returncode = 0
		if self._args.clean:
			self._clean()
		if self._args.lint_code:
			self._run_linter()
		else:
			self._run_tests()
		if self._args.coverage_scripts:
			self._coverage_scripts()
		if self._args.coverage or self._args.coverage_scripts:
			self._coverage_report()
		return self.returncode


parser = FriendlyArgumentParser(description = "tpp: Test Python Project wrapper for linting, coverage and testing of Python code")
parser.add_argument("--config-filename", metavar = "filename", default = ".tpp.conf", help = "Configuration filename. Defaults to %(default)s.")
parser.add_argument("-n", "--limit-failed-recording", metavar = "count", type = int, default = 1, help = "When tests are failing, on a subsequent run only those failed tests are re-run. This limits the amount of tests that are run. Defaults to %(default)d. When zero, all failed tests are recorded.")
parser.add_argument("-a", "--test-all", action = "store_true", help = "Even if a statefile exist, do not use it and test all (or specified) testcases.")
parser.add_argument("-d", "--show-test-duration", action = "store_true", help = "Show duration of test cases.")
parser.add_argument("-A", "--set-test-environment", action = "store_true", help = "Set environment variables that are specified in configuration (typically used to enable even slow tests that are usually excluded).")
parser.add_argument("-c", "--coverage", action = "store_true", help = "Run coverage tests as well.")
parser.add_argument("-C", "--coverage-scripts", action = "store_true", help = "Run coverage scripts after executing unit tests. Implies --coverage.")
parser.add_argument("-l", "--lint-code", action = "store_true", help = "Run linter instead of testing code.")
parser.add_argument("--clean", action = "store_true", help = "Remove statefile and coverage files before starting.")
parser.add_argument("-v", "--vim", action = "store_true", help = "Run vim with the linter fix commands if there are issues.")
parser.add_argument("testcase", nargs = "*", default = [ ], help = "Testcase modules/names to validate. Can be specified multiple times. By default, runs all tests when there were no previous failures and runs only previously failed tests if there were failures.")
args = parser.parse_args(sys.argv[1:])

sys.exit(TPP(args).run())
