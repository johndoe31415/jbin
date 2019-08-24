#!/usr/bin/python3
#	jbin - Joe's miscellaneous scripts, tools and configs
#	c: General-purpose scientific/cryptographic calculator
#	Copyright (C) 2009-2018 Johannes Bauer
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
import textwrap
import sys
import math
import random
import inspect
import time
import calendar
import datetime
import collections
import base64
import itertools
import string
from random import randint, randrange
from math import sqrt, sin, cos, tan, atan2, pi, e, exp, factorial, log, log10, ceil, floor
from fractions import Fraction
import urllib.parse

# Shortcuts here
ln = log
frc = Fraction
Frc = Fraction
query = " ".join(sys.argv[1:])

class FncDescription():
	def __init__(self, **kwargs):
		self._params = {
			"category":			kwargs.get("category"),
			"order":			kwargs.get("order"),
			"aliases":			kwargs.get("aliases", [ ]),
			"imports":			kwargs.get("imports", [ ]),
			"name":				None,
			"description":		None,
			"prototype":		None,
			"visible":			kwargs.get("visible", True),
		}

	@property
	def category(self):
		return self._params["category"]

	@property
	def order(self):
		if self._params["order"] is None:
			return self.name
		else:
			return self._params["order"]

	@property
	def aliases(self):
		return self._params["aliases"]

	@property
	def name(self):
		return self._params["name"]

	@property
	def description(self):
		return self._params["description"]

	@property
	def prototype(self):
		return self._params["prototype"]

	@property
	def visible(self):
		return self._params["visible"]

	@property
	def imports_satisfied(self):
		for importname in self._params["imports"]:
			try:
				m = __import__(importname)
			except ImportError:
				return False
		return True

	def _sortkey(self):
		return (self.category, self.order, self.name)

	def __lt__(self, other):
		return self._sortkey() < other._sortkey()

	def setname(self, name):
		self._params["name"] = name

	def setdescription(self, description):
		self._params["description"] = description

	def setprototype(self, prototype):
		self._params["prototype"] = prototype

class Matrix():
	def __init__(self, values):
		self._values = values
		assert(len(row) == len(self._values[0]) for row in self._values)

	@classmethod
	def colvector(cls, vector):
		return cls(list([ value ] for value in vector))

	@classmethod
	def rowvector(cls, vector):
		return cls([ vector ])

	@classmethod
	def zero(cls, cols, rows):
		result = [ ]
		for i in range(rows):
			result.append([ 0 ] * cols)
		return cls(result)

	@property
	def rows(self):
		return len(self._values)

	@property
	def cols(self):
		return len(self._values[0])

	def transpose(self):
		result = self.zero(self.rows, self.cols)
		for i in range(self.rows):
			for j in range(self.cols):
				result._values[j][i] = self._values[i][j]
		return result

	def __mul__(self, other):
		if self.cols != other.rows:
			raise ValueError("Cannot multiply a %d x %d matrix with a %d x %d matrix." % (self.rows, self.cols, other.rows, other.cols))
		result = Matrix.zero(other.cols, self.rows)
		for i in range(self.rows):
			for j in range(other.cols):
				for k in range(self.cols):
					result._values[i][j] += self._values[i][k] * other._values[k][j]
		return result

	def __setitem__(self, index, value):
		(i, j) = index
		self._values[i][j] = value

	def __getitem__(self, index):
		(i, j) = index
		return self._values[i][j]

	def dump(self):
		for row in self._values:
			col = " ".join("%7.3f" % (value) for value in row)
			print("( %s )" % (col))
		print()

	def __repr__(self):
		return str(self._values)

def log2(value):
	return log(value) / log(2)

def now() -> FncDescription(category = "Date/time"):
	"""Returns the current UTC time_t timestamp."""
	return time.time()

def rad2deg(x) -> FncDescription(category = "Trigonometric"):
	"""Convert radian angle to degrees."""
	return x * 180 / pi

def deg2rad(x) -> FncDescription(category = "Trigonometric"):
	"""Convert degree angle to radians."""
	return x / 180 * pi

def hms2deg(deg, mins, secs) -> FncDescription(category = "Trigonometric"):
	"""Convert degree/minute/second angle to degrees."""
	return deg + (mins / 60) + (secs / 3600)

def deg2hms(x) -> FncDescription(category = "Trigonometric"):
	"""Convert degree angle to degrees/minutes/seconds."""
	seconds = round(x * 3600)
	return [ seconds // 3600, seconds % 3600 // 60, seconds % 3600 % 60 ]

def flttobin(fltvalue, length) -> FncDescription(category = "Floating point"):
	"""\
	Converts the fractional part, i.e. a float value that is 0 <= x < 1, to
	a binary string representation of the requested length."""
	assert(0 <= fltvalue < 1)
	binstr = ""
	for i in range(length):
		fltvalue *= 2
		binstr += [ "0", "1" ][fltvalue >= 1.0]
		if fltvalue >= 1:
			fltvalue -= 1
	return binstr

def frcformat(fraction, length = 20) -> FncDescription(category = "Floating point"):
	"""\
	Converts a fraction to a decimal fraction with the required precision."""

	orig = fraction

	digits = [ ]
	while fraction > 1:
		digit = int(fraction % 10)
		fraction = fraction / 10
		digits.append(digit)
	precomma = "".join([ str(digit) for digit in digits[::-1] ])

	digits = [ ]
	fraction = orig
	for i in range(length):
		fraction = fraction * 10
		digit = int(fraction % 10)
		digits.append(digit)
	postcomma = "".join([ str(digit) for digit in digits ])
	return "%s.%s" % (precomma, postcomma)

def pick_arb_flt(floatvalue, exponentbits, mantissabits) -> FncDescription(category = "Floating point"):
	"""Picks apart an arbitrary float value (float or str -> dict)."""
	if isinstance(floatvalue, str):
		floatvalue = Fraction(floatvalue)
	signbit = (floatvalue < 0)
	if floatvalue < 0:
		floatvalue = -floatvalue
	integral = int(floatvalue)
	fraction = floatvalue - integral

	if integral <= 1:
		exponent = 0
		binint = ""
		if integral == 0:
			while fraction < 1:
				fraction *= 2
				exponent -= 1
			fraction -= 1
	else:
		binint = bin(integral)[3:]
		exponent = len(binint)
	binfrc = flttobin(fraction, mantissabits)

	mantissaval = int((binint + binfrc)[:mantissabits], 2)

	signval = [ 0, 1 ][signbit]
	return { "sign": signval, "exponent": exponent, "mantissa": mantissaval }

def make_arb_flt(exponentbits, mantissabits, sign, exponent, mantissa) -> FncDescription(category = "Floating point"):
	"""Creates an arbitrary float value (tuple -> float)."""
	assert((sign == 0) or (sign == 1))
	exponent += (2 ** (exponentbits - 1)) - 1
	value = (sign << (exponentbits + mantissabits)) | (exponent << mantissabits) | (mantissa << 0)
	return value

def encode_arb_flt(floatvalue, exponentbits, mantissabits) -> FncDescription(category = "Floating point"):
	"""Encodes an arbitrary float value (float -> int)."""
	picked = pick_arb_flt(floatvalue, exponentbits, mantissabits)
	sign = picked["sign"]
	exponent = picked["exponent"]
	mantissa = picked["mantissa"]

	return make_arb_flt(exponentbits, mantissabits, sign, exponent, mantissa)

def decode_arb_flt(intvalue, exponentbits, mantissabits) -> FncDescription(category = "Floating point"):
	"""Decodes an arbitrary float value (int -> Fraction)"""
	assert(isinstance(intvalue, int))
	assert(isinstance(exponentbits, int))
	assert(isinstance(mantissabits, int))
	signbit = (intvalue & (1 << (exponentbits + mantissabits))) != 0
	exponentmask = ((1 << exponentbits) - 1) << mantissabits
	mantissamask = ((1 << mantissabits) - 1) << 0
	exponentval = (intvalue & exponentmask) >> mantissabits
	exponent = exponentval - (2 ** (exponentbits - 1)) + 1
	mantissa = intvalue & mantissamask
	value = (2 ** exponent) * (1 + Fraction(mantissa / ((1 << mantissabits) - 1)))
	if signbit:
		value = -value
	return value

def encodeflt(floatvalue) -> FncDescription(category = "Floating point"):
	"""Encodes a 32-bit float (float -> int)."""
	return encode_arb_flt(floatvalue, 8, 23)

def encodedbl(floatvalue) -> FncDescription(category = "Floating point"):
	"""Encodes a 64-bit double (float -> int)."""
	return encode_arb_flt(floatvalue, 11, 52)

def decodeflt(intvalue) -> FncDescription(category = "Floating point"):
	"""Decodes a 32-bit float value (int -> float)."""
	return decode_arb_flt(intvalue, 8, 23)

def decodedbl(intvalue) -> FncDescription(category = "Floating point"):
	"""Decodes a 64-bit double value (int -> float)."""
	return decode_arb_flt(intvalue, 11, 52)

def pickflt(floatvalue) -> FncDescription(category = "Floating point"):
	"""Picks a 32-bit float apart (float -> dict)."""
	return pick_arb_flt(floatvalue, 8, 23)

def pickdbl(floatvalue) -> FncDescription(category = "Floating point"):
	"""Picks a 64-bit double apart (float -> dict)."""
	return pick_arb_flt(floatvalue, 11, 52)

def mkflt(sign, exponent, mantissa) -> FncDescription(category = "Floating point"):
	"""Creates a 32-bit float value (tuple -> int)."""
	return make_arb_flt(8, 23, sign, exponent, mantissa)

def mkdbl(sign, exponent, mantissa) -> FncDescription(category = "Floating point"):
	"""Creates a 64-bit double value (tuple -> int)."""
	return make_arb_flt(11, 52, sign, exponent, mantissa)

def bo(x, bytecnt = 0) -> FncDescription(category = "Endianness"):
	"""\
	Switches the byteorder of the given number (i.e. most significant byte
	becomes least significant byte and vice versa). Can optionally receive a
	minimum byte count."""
	splitint = [ ]
	while (x > 0) or (bytecnt > 0):
		splitint.append(x & 0xff)
		x >>= 8
		bytecnt -= 1
	x = sum([ splitint[len(splitint) - 1 - i] * (256 ** i) for i in range(len(splitint)) ])
	return x

def bitmask(lo, hi) -> FncDescription(category = "Bit operations", aliases = [ "bitlen" ]):
	"""Returns a bitmask that includes bits lo:hi (both inclusive)."""
	(lo, hi) = (min(lo, hi), max(lo, hi))
	length = hi - lo + 1
	mask = (1 << length) - 1
	return mask << lo

def highbit(x) -> FncDescription(category = "Bit operations", aliases = [ "bitlen" ]):
	"""Returns the highest bit number of the integer input."""
	assert(isinstance(x, int))
	assert(x > 0)
	return x.bit_length() - 1

def bitcnt(x) -> FncDescription(category = "Bit operations", aliases = [ "bitcount", "hweight" ]):
	"""\
	Returns the number of set bits in the given value which is also known as
	the Hamming Weight of a number."""
	assert(isinstance(x, int))
	assert(x >= 0)
	cnt = 0
	while x > 0:
		if (x & 1) == 1:
			cnt += 1
		x >>= 1
	return cnt

def parity(x) -> FncDescription(category = "Bit operations"):
	"""Returns the parity bit of a given value."""
	return bitcnt(x) % 2

def ror(value, rotate, regwidth = 32) -> FncDescription(category = "Bit operations"):
	"""\
	Returns the result of a rotate right operation with a specified
	register width."""
	assert(isinstance(regwidth, int))
	assert(regwidth >= 1)
	rotate = rotate % regwidth
	if rotate == 0:
		return value
	lomask = (1 << rotate) - 1
	himask = ~lomask
	return ((value & lomask) << (regwidth - rotate)) | ((value & himask) >> rotate)

def rol(value, rotate, regwidth = 32) -> FncDescription(category = "Bit operations"):
	"""\
	Returns the result of a rotate left operation with a specified
	register width."""
	return ror(value, regwidth - rotate, regwidth)

def bitlist(x) -> FncDescription(category = "Bit operations"):
	"""Returns the bit numbers of set bits in the given value."""
	assert(isinstance(x, int))
	assert(x >= 0)
	bitno = 0
	setbits = [ ]
	while x > 0:
		if (x & 1) == 1:
			setbits.append(bitno)
		x >>= 1
		bitno += 1
	return setbits

def bito(x, bitcount = 8) -> FncDescription(category = "Bit operations"):
	"""\
	Switches bitorder of the given number (i.e. most significant bit becomes
	least significant bit and vice versa)."""
	y = 0
	for i in range(bitcount):
		if (x & (1 << i)) != 0:
			y |= (1 << (bitcount - 1 - i))
	return y

def rbitlist(bitlist) -> FncDescription(category = "Bit operations"):
	"""Converts a list of bits that is supposed to be set into an integer."""
	result = 0
	for bit in bitlist:
		result |= (1 << bit)
	return result

def sqr(x) -> FncDescription(category = "Number theoretic functions"):
	"""Sqares the given number."""
	return x * x

def intsqrt(x) -> FncDescription(category = "Number theoretic functions"):
	"""\
	Does a binary search to find the integer square root (lower and upper
	bound)."""
	assert(isinstance(x, int))
	assert(x >= 6)
	(low, high) = (1, x // 2)
	while (high - low) >= 2:
		mid = (high + low) // 2
		midsqr = sqr(mid)
		if midsqr > x:
			high = mid
		elif midsqr < x:
			low = mid
		else:
			return (mid, mid)
	return (low, high)

def perfsqrt(x) -> FncDescription(category = "Number theoretic functions"):
	"""\
	Gives the perfect integer square of a number and throws an exception if no
	such number exists."""
	(low, high) = intsqrt(x)
	if low != high:
		raise Exception("%d has no perfect square root." % (x))
	return low

def gcd(a, b, *more) -> FncDescription(category = "Number theoretic functions"):
	"""Caluclates the greatest common divisor of two or more values."""
	if len(more) != 0:
		return gcd(gcd(a, b), *more)
	assert(a >= 0)
	assert(b >= 0)
	assert(isinstance(a, int))
	assert(isinstance(b, int))
	if a == 0:
		return b
	while b > 0:
		(a, b) = (b, a % b)
	return a

def lcm(a, b) -> FncDescription(category = "Number theoretic functions"):
	"""Calculates the least common multiple of two values."""
	return abs(a * b) // gcd(a, b)

def egcd(a, b) -> FncDescription(category = "Number theoretic functions"):
	"""Calculates the extended GCD of two numbers."""
	if a == 0:
		return (b, 0, 1)
	else:
		g, y, x = egcd(b % a, a)
		return (g, x - (b // a) * y, y)

def modinv(a, m) -> FncDescription(category = "Number theoretic functions"):
	"""Calculates the modular inverse of a mod m"""
	(g, x, y) = egcd(a, m)
	if g != 1:
		raise Exception("Modular inverse of %d mod %d does not exist" % (a, m))
	else:
		return x % m

def isprime(n) -> FncDescription(category = "Prime numbers"):
	"""Test whether n is prime using the Miller-Rabin primary test."""
	assert(isinstance(n, int))
	if n in (2, 3, 5, 7, 11, 13, 17, 19, 23, 29):
		return True
	elif (n < 29) or ((n % 2) == 0):
		return False

	for s in range(n.bit_length(), 0, -1):
		if (n - 1) % (2 ** s) == 0:
			break
	d = (n - 1) // (2 ** s)
	assert((d % 2) == 1)
	assert(n - 1 == d * (2 ** s))

	for iteration in range(20):
		a = random.randint(2, n - 2)
		if pow(a, d, n) == 1:
			continue

		passed = False
		for r in range(s):
			v = pow(a, d * (2 ** r), n)
			if v == n - 1:
				passed = True
				break

		if not passed:
			return False

	return True

def nextprime(n) -> FncDescription(category = "Prime numbers"):
	"""Generates the next larger prime that is greater or equal to n."""
	n = (n & ~0x1) + 1
	while True:
		if isprime(n):
			return n
		n += 2

def nthprime(n) -> FncDescription(category = "Prime numbers"):
	"""Generates the n-th prime number. For n = 1, it returns 2."""
	assert(isinstance(n, int))
	assert(n >= 1)
	c = 1
	i = 3
	if n == 1:
		return 2
	while True:
		if isprime(i):
			c += 1
			if n == c:
				return i
		i += 2

def randbytes(length, secure = False) -> FncDescription(category = "Random numbers"):
	"""\
	Retrieves a number of bytes from /dev/urandom (or /dev/random if the
	'secure' keyword is set to True)."""
	assert(isinstance(length, int))
	if secure:
		filename = "/dev/random"
	else:
		filename = "/dev/urandom"
	f = open(filename, "rb")
	return f.read(length)

def randintbits(bits) -> FncDescription(category = "Random numbers"):
	"""\
	Generates a random number with 'bits' bit, i.e. from within the range of [
	2 ^ (bits - 1); (2 ^ bits) - 1 ]"""
	assert(isinstance(bits, int))
	assert(bits >= 2)
	minval = 2 ** (bits - 1)
	maxval = (2 ** bits) - 1
	value = randint(minval, maxval)
	assert(value.bit_length() == bits)
	return value

def randprime(maxval) -> FncDescription(category = "Prime numbers"):
	"""Generates a random prime number from the range of [ 2; maxval ]"""
	assert(isinstance(maxval, int))
	assert(maxval >= 2)
	while True:
		p = randint(2, maxval) | 0x01
		if isprime(p):
			return p

def randprimebits(bits, both_msb = False) -> FncDescription(category = "Prime numbers"):
	"""\
	Generates a random prime number with 'bits' bit, i.e. from within the range
	of [ 2 ^ (bits - 1) + 1; (2 ^ bits) - 1 ]"""
	assert(isinstance(bits, int))
	assert(bits >= 2)
	while True:
		p = randintbits(bits) | 0x01
		if both_msb:
			# Also set the second highest bit
			p |= (1 << (bits - 2))
		if isprime(p):
			assert(p.bit_length() == bits)
			return p

def randprimebits_x_mod_y(bits, x, y, both_msb = False) -> FncDescription(category = "Prime numbers"):
	"""\
	Generates a random prime number with 'bits' bit, i.e. from within the range
	of [ 2 ^ (bits - 1) + 1; (2 ^ bits) - 1 ] that has the value x modulo y."""
	while True:
		p = randprimebits(bits = bits, both_msb = both_msb)
		if (p % y) == x:
			return p

def randsafeprimebits(bits) -> FncDescription(category = "Prime numbers"):
	"""\
	Generates a random safe prime number with 'bits' bit, i.e. from within the
	range of [ 2 ^ (bits - 1) + 1; (2 ^ bits) - 1 ] which satisfies the
	constraint that p = 2q + 1 where q is also prime"""
	assert(isinstance(bits, int))
	assert(bits >= 3)
	while True:
		q = randintbits(bits - 1) | 0x01
		if isprime(q):
			p = 2 * q + 1
			if isprime(p):
				assert(p.bit_length() == bits)
				return p

__RSAParams = collections.namedtuple("RSAParams", [ "p", "q", "n", "phin", "e", "d", "gcdp1q1" ])

def rsagen(p = None, q = None, bits = 64, e = 65537, strict = True, safe = True) -> FncDescription(category = "Cryptography"):
	"""\
	Creates RSA parameters in with p and q primes may be pre-given or a
	bitlength of the prime numbers may be given. In strict mode (default on) p
	and q are checked for primality before using them. In safe mode (default
	also on) the greatest common divisor of (p - 1) and (q - 1) must be equal
	to 2."""

	if (p is not None) and (strict) and (not isprime(p)):
		raise Exception("Strict mode is active, p was given but it is not prime.")
	if (q is not None) and (strict) and (not isprime(q)):
		raise Exception("Strict mode is active, q was given but it is not prime.")

	for tries in range(1000):
		if p is None:
			chosen_p = randprimebits(bits // 2, both_msb = True)
		else:
			chosen_p = p

		if q is None:
			chosen_q = randprimebits(bits - chosen_p.bit_length(), both_msb = True)
		else:
			chosen_q = q

		if strict and (p is None) and (q is None) and (chosen_q == chosen_p):
			continue

		n = chosen_p * chosen_q
		phin = (chosen_p - 1) * (chosen_q - 1)
		if gcd(phin, e) != 1:
			# No RSA parameters found with this p/q combination, try again
			continue

		d = modinv(e, phin)
		gcdp1q1 = gcd(chosen_p - 1, chosen_q - 1)
		if safe and (gcdp1q1 != 2):
			continue
		break
	else:
		if safe:
			raise Exception("With given constraints, no RSA generation was possible with reasonable effort. Try deactivating safe mode.")
		else:
			raise Exception("With given constraints, no RSA generation was possible with reasonable effort.")
	return __RSAParams(chosen_p, chosen_q, n, phin, e, d, gcdp1q1)

def rsa2der(rsaparams) -> FncDescription(category = "Cryptography", imports = [ "pyasn1" ]):
	"""\
	Converts previously generated RSA parameters (rsagen) to DER ASN.1
	representation. Calculates intermediate values required for RSA-CRT as
	well."""
	import pyasn1.codec.der.encoder
	import pyasn1.type.univ

	exp1 = rsaparams.d % (rsaparams.p - 1)
	exp2 = rsaparams.d % (rsaparams.q - 1)
	coeff = modinv(rsaparams.q, rsaparams.p)

	key = pyasn1.type.univ.Sequence()
	key.setComponentByPosition(0, pyasn1.type.univ.Integer(0))
	key.setComponentByPosition(1, pyasn1.type.univ.Integer(rsaparams.n))
	key.setComponentByPosition(2, pyasn1.type.univ.Integer(rsaparams.e))
	key.setComponentByPosition(3, pyasn1.type.univ.Integer(rsaparams.d))
	key.setComponentByPosition(4, pyasn1.type.univ.Integer(rsaparams.p))
	key.setComponentByPosition(5, pyasn1.type.univ.Integer(rsaparams.q))
	key.setComponentByPosition(6, pyasn1.type.univ.Integer(exp1))
	key.setComponentByPosition(7, pyasn1.type.univ.Integer(exp2))
	key.setComponentByPosition(8, pyasn1.type.univ.Integer(coeff))
	return pyasn1.codec.der.encoder.encode(key)

def derfile2rsa(filename) -> FncDescription(category = "Cryptography", imports = [ "pyasn1" ]):
	"""\
	Opens the given file, reads the contents and interprets it as an ASN.1 DER
	encoded RSA private key. Converts that to a RSA key tuple."""
	import pyasn1.codec.der.decoder
	import pyasn1.type.univ

	with open(filename, "rb") as f:
		der = f.read()
	(asn1, tail) = pyasn1.codec.der.decoder.decode(der)

	(n, e, d, p, q) = (int(asn1[1]), int(asn1[2]), int(asn1[3]), int(asn1[4]), int(asn1[5]))
	phin = (p - 1) * (q - 1)
	gcdp1q1 = gcd(p - 1, q - 1)
	return __RSAParams(p, q, n, phin, e, d, gcdp1q1)

def ts2timet(ts) -> FncDescription(category = "Date/time"):
	"""Converts a string UTC timestamp in ISO format to a time_t value."""
	ts = datetime.datetime.strptime(ts, "%Y-%m-%d %H:%M:%S")
	return calendar.timegm(ts.utctimetuple())

def timet2ts(timet) -> FncDescription(category = "Date/time"):
	"""Converts a time_t time value to a ISO formatted UTC datetime string."""
	return datetime.datetime.utcfromtimestamp(timet).strftime("%Y-%m-%d %H:%M:%S")

def strpdate(datestr) -> FncDescription(category = "Date/time"):
	"""Parses a ISO formatted date string YYYY-mm-dd to a datetime object."""
	return datetime.datetime.strptime(datestr, "%Y-%m-%d")

def strpdatetime(datetimestr) -> FncDescription(category = "Date/time"):
	"""\
	Parses a ISO formatted datetime string YYYY-mm-dd HH:MM:SS to a datetime
	object."""
	return datetime.datetime.strptime(datestr, "%Y-%m-%d %H:%M:%S")

def digsum(value, base = 10) -> FncDescription(category = "Integer representation"):
	"""Gives the digit sum at the given base. Default base is 10."""
	dsum = 0
	while value > 0:
		dsum += (value % base)
		value //= base
	return dsum

def uint(value, bits) -> FncDescription(category = "Integer representation"):
	"""Converts a value to an unsigned integer of the specified bitlength."""
	mask = (1 << bits) - 1
	return value & mask

def sint(value, bits) -> FncDescription(category = "Integer representation", order = "int00"):
	"""\
	Converts a value to a signed integer of the specified bitlength using twos
	complements notation."""
	value = uint(value, bits)
	half = 1 << (bits - 1)
	if value >= half:
		value = -((1 << bits) - value)
	return value

def int32(value) -> FncDescription(category = "Integer representation"):
	"""\
	Converts a value to a 32 bit signed integer using twos complements
	notation."""
	return sint(value, 32)

def int16(value) -> FncDescription(category = "Integer representation"):
	"""\
	Converts a value to a 16 bit signed integer using twos complements
	notation."""
	return sint(value, 16)

def int8(value) -> FncDescription(category = "Integer representation", order = "int08"):
	"""\
	Converts a value to a 8 bit signed integer using twos complements
	notation."""
	return sint(value, 8)

def uint8(value) -> FncDescription(category = "Integer representation", order = "uint08"):
	"""Converts a value to an unsigned integer of 8 bits."""
	return uint(value, 8)

def uint16(value) -> FncDescription(category = "Integer representation"):
	"""Converts a value to an unsigned integer of 16 bits."""
	return uint(value, 16)

def uint32(value) -> FncDescription(category = "Integer representation"):
	"""Converts a value to an unsigned integer of 32 bits."""
	return uint(value, 32)

def cuint128(value) -> FncDescription(category = "Integer representation"):
	"""\
	Converts a integer value of 128 bit to two 64-bit integers (high and low
	word)."""
	assert(value.bit_length() <= 128)
	high = (value >> 64) & ((1 << 64) - 1)
	low = (value >> 0) & ((1 << 64) - 1)
	return "0x%x, 0x%x" % (high, low)

def primesbelow(n) -> FncDescription(category = "Prime numbers"):
	"""Returns all prime numbers below a given number n."""
	if n <= 2:
		return [ ]
	result = [ 2 ]
	for i in range(3, n, 2):
		if isprime(i):
			result.append(i)
	return result

def pollard_rho(n) -> FncDescription(category = "Prime numbers"):
	"""\
	Calculate Pollards Rho for a given number n. Returns either a prime
	factor of n or None."""
	def f(x, n):
		return ((x * x) + 1) % n

	(x, y, d) = (2, 2, 1)
	while d == 1:
		x = f(x, n)
		y = f(f(y, n), n)
		d = gcd(abs(x - y), n)
	if 1 < d < n:
		return d
	else:
		assert(d == n)
		return None

def factor(n) -> FncDescription(category = "Prime numbers"):
	"""\
	Gives the prime factor decomposition of a given number as a
	dictionary."""
	factors = { }
	while n > 1:
		factor = pollard_rho(n)
		if factor is None:
			break

		factors[factor] = 0
		while (n % factor) == 0:
			factors[factor] += 1
			n //= factor
	if factor is None:
		factors[n] = 1
	return factors

def primefactors(n) -> FncDescription(category = "Prime numbers"):
	"""Gives the prime factor decomposition of a given number as a list."""
	return list(factor(n).keys())

def factorlist(n) -> FncDescription(category = "Prime numbers"):
	"""\
	Gives the prime factor decomposition of a given number as a list of
	prime factors."""
	factorization = factor(n)
	factorlist = [ ]
	for (prime, count) in sorted(factorization.items()):
		factorlist += [ prime ] * count
	return factorlist

def divisors(n) -> FncDescription(category = "Prime numbers"):
	"""Return all values that divide n."""
	assert(isinstance(n, int))

	factors = tuple(factor(n).items())
	values = tuple(element[0] for element in factors)
	maxcounts = tuple(element[1] for element in factors)

	divisors = set()
	countiters = [ range(count + 1) for count in maxcounts ]
	for counts in itertools.product(*countiters):
		terms = [ value ** exponent for (value, exponent) in zip(values, counts) ]
		divisors.add(product(terms))

	return sorted(list(divisors))

def totient(n):
	"""Calculates the Euler Totient function of a given number."""
	if n == 0: return 1
	tot = 1
	for p, exp in factor(n).items():
		tot *= (p - 1) *  (p ** (exp - 1))
	return tot

def lba2dd(lba, lbablocksize = 512, ddblocksize = 1024 * 1024, ddcount = 500) -> FncDescription(category = "Disk size operations"):
	"""\
	Gives coordinates for dd so that the given LBA block is exactly centered in
	the count window."""
	DDParams = collections.namedtuple("DDParams", [ "ddblocksize", "ddread_skip", "ddwrite_seek", "ddcount" ])
	offset = lba * lbablocksize
	ddblock = round(offset / ddblocksize)
	ddskip = round(ddblock - (ddcount / 2))
	if ddskip < 0:
		ddskip = 0
	return DDParams(ddblocksize = ddblocksize, ddread_skip = ddskip, ddwrite_seek = ddskip, ddcount = ddcount)

def chs2lba(hpc, spt, c, h, s) -> FncDescription(category = "Disk size operations"):
	"""\
	Converts a position (c, h, s) on a disk with geometry (hpc, spt) to a
	logical block address. HPT stands for heads/track and SPT for
	sectors/track."""
	assert(isinstance(hpc, int) and (hpc > 0))
	assert(isinstance(spt, int) and (spt > 0))
	assert(isinstance(c, int) and (c >= 0))
	assert(isinstance(h, int) and (h >= 0))
	assert(isinstance(s, int) and (s >= 0))
	return (((c * hpc) + h) * spt) + s - 1

def chs2pos(hpc, spt, c, h, s, secsize = 512) -> FncDescription(category = "Disk size operations"):
	"""\
	Converts a position (c, h, s) on a disk with geometry (hpc, spt, secsize)
	to a byte position on disk. HPT stands for heads/track and SPT for
	sectors/track, sector size defaults to 512 bytes/sector."""
	assert(isinstance(secsize, int) and (secsize > 0))
	return secsize * chs2lba(hpc, spt, c, h, s)

def ddblocksize(size) -> FncDescription(category = "Disk size operations"):
	"""\
	Convert a size in bytes to a block size and block count value that dd
	will accept. Block size will not exceed 1 MiB."""
	DDParams = collections.namedtuple("DDParams", [ "blocksize", "count" ])
	blocksize = 1
	count = size
	while ((count % 2) == 0) and (blocksize < (1024 * 1024)):
		blocksize *= 2
		count //= 2
	if (blocksize % (1024 * 1024)) == 0:
		blocksize = "%dM" % (blocksize // (1024 * 1024))
	elif (blocksize % 1024) == 0:
		blocksize = "%dk" % (blocksize // (1024))
	return DDParams(blocksize = blocksize, count = count)

def ahex2int(ahex) -> FncDescription(category = "Binary data manipulation"):
	"""\
	Converts an ASCII hex value (i.e. one where a = 0, b = 1, c = 2, ...,
	p = 15 to an integer value."""
	alphabet = "abcdefghijklmnop"
	ahex = ahex.lower()
	value = 0
	for char in ahex:
		nibble = alphabet.index(char)
		value = (value << 4) | nibble
	return value

def xor(x, y) -> FncDescription(category = "Binary data manipulation"):
	"""Perform XOR of two byte arrays."""
	return bytes(cx ^ cy for (cx, cy) in zip(x, y))

def bytes2hex(data) -> FncDescription(category = "Binary data manipulation"):
	"""Converts a byte data array to a hex string."""
	assert(isinstance(data, bytes))
	return "".join("%02x" % (c) for c in data)

def filetohex(filename) -> FncDescription(category = "Binary data manipulation"):
	"""Reads a file in and dumps the hexadecimal string."""
	return bytes2hex(open(filename, "rb").read())

def b64enc(data) -> FncDescription(category = "Binary data manipulation"):
	"""Performs base64 encoding on the given data."""
	return base64.b64encode(data).decode("utf-8")

def b64dec(data) -> FncDescription(category = "Binary data manipulation"):
	"""Performs base64 decoding on the given data."""
	return base64.b64decode(data.encode("utf-8"))

def b64uenc(data) -> FncDescription(category = "Binary data manipulation"):
	"""Performs base64url encoding on the given data."""
	b64data = base64.b64encode(data).decode("utf-8")
	b64data = b64data.rstrip("=")
	b64data = b64data.replace("+", "-")
	b64data = b64data.replace("/", "_")
	return b64data

def b64udec(data) -> FncDescription(category = "Binary data manipulation"):
	"""Performs base64url decoding on the given data."""
	data = data.replace("-", "+")
	data = data.replace("_", "/")
	data = data.replace("\n", "")
	data = data.replace(" ", "")
	if len(data) % 4 != 0:
		data += ("=" * (4 - (len(data) % 4)))
	return base64.b64decode(data.encode("utf-8"))

def binout(data) -> FncDescription(category = "Binary data manipulation"):
	"""Write binary data directly to stdout."""
	sys.stdout.buffer.write(data)

def carray(data, linelength = 100) -> FncDescription(category = "Binary data manipulation"):
	"""Formats a chunk of bytes data so it can be used as a C array."""
	assert(isinstance(data, bytes))
	assert(isinstance(linelength, int))
	datastr = ", ".join([ "0x%02x" % (c) for c in data ])
	wrapped = textwrap.wrap(datastr, width = linelength, initial_indent = "", subsequent_indent = "\t\t\t ")
	wrapped[0] = "const uint8_t data[] = { " + wrapped[0]
	wrapped[-1] += " };"
	return "\n".join(wrapped)

def bytes2int(data) -> FncDescription(category = "Binary data manipulation"):
	"""Converts a byte data array into an integer."""
	assert(isinstance(data, bytes))
	return sum(c << (8 * index) for (index, c) in enumerate(data))

def int2bytes(data, length = None) -> FncDescription(category = "Binary data manipulation"):
	"""Converts an integer into a byte array."""
	assert(isinstance(data, int))
	result = [ ]
	while ((length is None) and (data > 0)) or ((length is not None) and (length > 0)):
		result.append(data & 0xff)
		data >>= 8
		if length is not None:
			length -= 1
	return bytes(result)

def hexstr(value) -> FncDescription(category = "Binary data manipulation"):
	"""Converts a integer value to a bytewise hex string."""
	assert(isinstance(value, int))
	assert(value >= 0)
	return " ".join("%02x" % (c) for c in reversed(int2bytes(value)))

def product(iteratable) -> FncDescription(category = "Misc", aliases = [ "prod" ]):
	"""Returns the product of all values in the iteratable."""
	value = 1
	for element in iteratable:
		value *= element
	return value

def spacing(length, count, endsincluded = False) -> FncDescription(category = "Misc"):
	"""\
	Spaces out points on a line evenly. Can choose if you want the ends to
	be included or not."""
	if not endsincluded:
		distance = length / (count + 1)
		spacing = [ (distance * i) for i in range(1, count + 1) ]
	else:
		distance = length / (count - 1)
		spacing = [ (distance * i) for i in range(count) ]
	return spacing

def inchfrac(mm, tolpercent = 1.5) -> FncDescription(category = "Misc"):
	"""\
	Converts a millimeter length to a fractional length with the specified
	tolerance."""
	inches = mm / 25.4
	tolerance = tolpercent / 100 * mm
	denominator = 2
	for i in range(10):
		numerator = round(inches * denominator)
		real = numerator / denominator * 25.4
		error = abs(mm - real)
		if (numerator != 0) and (error < tolerance):
			return Fraction(numerator, denominator)
		denominator *= 2

def inchfracs(mm) -> FncDescription(category = "Misc"):
	"""\
	Converts a millimeter length to all relevant fractional lengths and
	outputs a list with percentage errors."""
	inches = mm / 25.4
	denominator = 2
	fractions = set()
	for i in range(10):
		numerator = round(inches * denominator)
		real = numerator / denominator * 25.4
		error = abs(mm - real)
		if numerator != 0:
			fractions.add(Fraction(numerator, denominator))
		denominator *= 2
	fractions = list(fractions)
	fractions.sort(key = lambda f: f.denominator)

	return [ (fract, -100 * ((float(fract) * 25.4) - mm) / mm) for fract in fractions ]

def nPr(n, r) -> FncDescription(category = "Stochastical functions"):
	"""Gives the number of permutations of r out of n."""
	return math.factorial(n) // math.factorial(n - r)

def nCr(n, r) -> FncDescription(category = "Stochastical functions"):
	"""Gives the number of combinations of r out of n."""
	return nPr(n, r) // math.factorial(r)

def ccp_ev(n) -> FncDescription(category = "Stochastical functions"):
	"""Gives the expected value of the coupon collector's problem: where there
	are n types of coupons, what is the number of expected draws t one must
	make to draw at least one of every of the n types of coupons."""
	gamma = 0.5772156649		 # Euler-Mascheroni constant
	ET = n * math.log(n) + gamma * n + 0.5
	return ET

def ccp_dist(n, t) -> FncDescription(category = "Stochastical functions"):
	"""Gives the probabilities of the coupon collector's problem: where there
	are n types of coupons and t draws, what are the probabilites after those t
	draws to receive 0, 1, 2, ... n different types of token."""

	# Generate Markov chain matrix
	M = Matrix.zero(n + 1, n + 1)
	M[(1, 0)] = 1
	for i in range(n):
		p =  (i + 1) / n
		M[(i + 1, i + 1)] = p
		if i < n - 1:
			M[(i + 2, i + 1)] = 1 - p

	# Initial probability vector
	v = Matrix.colvector([ 1 ] + ([ 0 ] * n))

	# Repeatedly multiply matrix with vector
	for q in range(t):
		v = M * v

	# Extract proability distribution
	v = v.transpose()._values[0]
	return v

def ccp(n, t) -> FncDescription(category = "Stochastical functions"):
	"""Gives the probability of the coupon collector's problem: where there are
	n types of coupons and t draws, what is the probabilitiy that among the t
	draws every of the n types of coupon is present at least once."""

	probabilities = ccp_dist(n, t)
	# We're looking for all n coupons
	return probabilities[n]

def ccp_hp(n, t) -> FncDescription(category = "Stochastical functions"):
	"""Gives the number of different coupon occurrences that have the highest
	probability of appearing for n types of coupons after t tries."""

	probabilities = ccp_dist(n, t)

	# We're looking for the index with highest probability
	high_index = 0
	for (index, value) in enumerate(probabilities):
		if probabilities[index] > probabilities[high_index]:
			high_index = index

	return high_index

def binom(n, p, min_p = 0.001) -> FncDescription(category = "Stochastical functions"):
	"""\
	For n individuals with a probability of p each, give the probability
	that the given event occurs exactly i times, where i = [ 0; n ]."""
	for i in range(n + 1):
		q = (p ** i) * ((1 - p) ** (n - i)) * nCr(n, i)
		if q >= min_p:
			print("%4d %4.1f%%" % (i, 100 * q))

def normal(x, mu, sigma) -> FncDescription(category = "Stochastical functions"):
	"""\
	Returns the value of the Gaussian normal distribution at point x for the
	given values mu and sigma."""
	return 1 / (sigma * math.sqrt(2 * math.pi)) * math.exp(-(((x - mu) ** 2) / (2 * sigma ** 2)))

def erf(x) -> FncDescription(category = "Stochastical functions"):
	"Returns an approximation of the value of the error function at point x."
	if x < 0:
		return -erf(-x)
	a = (8 * (math.pi - 3)) / (3 * math.pi * (4 - math.pi))
	return math.sqrt(1 - math.exp(-(x ** 2) * (4 / math.pi + a * x ** 2) / (1 + a * x ** 2)))

def cdf(x, mu, sigma) -> FncDescription(category = "Stochastical functions"):
	"""\
	Returns the value of the cumulative distribution function (CDF) of the
	Gaussian normal function at point x for given mu and sigma."""
	return 1 / 2 * (1 + erf((x - mu) / (sigma * math.sqrt(2))))

def probability_for_delta(mu, delta, sigma) -> FncDescription(category = "Stochastical functions"):
	"""\
	Returns the probability for a given distribution mu, sigma that a value
	lies within the interval [mu - delta, mu + delta]."""
	return cdf(mu + delta, mu, sigma) - cdf(mu - delta, mu, sigma)

def sigma_to_probability(sigma) -> FncDescription(category = "Stochastical functions"):
	"""\
	Gives for a given multiple of sigma the associated probability that the
	value is within the interval [mu - sigma, mu + sigma]."""
	return probability_for_delta(0, sigma, 1)

def probability_to_sigma(p) -> FncDescription(category = "Stochastical functions"):
	"""\
	Gives for a given probability the corresponding critical value, i.e. the
	sigma multiple required to achieve that probability."""
	assert(0 < p < 1)
	low = 0
	high = 20
	last = None
	for i in range(100):
		mid = (low + high) / 2
		x = sigma_to_probability(mid)
		if x > p:
			high = mid
		else:
			low = mid
		if (last is not None) and (abs(last - x) < 1e-9):
			break
		last = x
	else:
		raise Exception("Couldn't get a valid estimate for sigma.")
	return mid


def urldecode(uri) -> FncDescription(category = "String functions"):
	"""Decodes an URI."""
	return urllib.parse.unquote(uri)

def pwgen_bits(bitcnt, ambiguous = False, special = False) -> FncDescription(category = "Misc"):
	"""\
	Generates a password with at least n bits. Can include special
	characters (to create shorter password) and can remove ambiguous characters
	as well (such as O and 0 or l and 1)."""
	bytecnt = (bitcnt + 7) // 8
	data = os.urandom(bytecnt)
	data_int = bytes2int(data)
	if ambiguous:
		alphabet = string.ascii_uppercase + string.ascii_lowercase + string.digits
	else:
		alphabet = "ABCDEFHJLMNPQRSTUVWXYZabcdefghijmnpqrstwxyz2345789"
	if special:
		if ambiguous:
			alphabet += "+-_*.!()[]%$"
		else:
			alphabet += "+-_*.!%$"
	password = ""
	while data_int > 0:
		(data_int, charno) = divmod(data_int, len(alphabet))
		password += alphabet[charno]
	return password

def cvalue(value) -> FncDescription(category = "Misc"):
	"""\
	Takes a component input value and converts it to its pedant. E.g., 104
	is read as 10 * 10^4 and for capacitors gives the capacitance in Picofarad.
	1002 in turn means 100 * 10^2 (i.e., for resistors this would equal 10
	kOhm, for inductivities 10 mH). In particular the coefficient is 1 for
	resistors, 1e-12 for capacitos and 1e-6 for inductivities."""
	exponent = value % 10
	base = value // 10
	return base * (10 ** exponent)

def unit(value, fromunit, tounit) -> FncDescription(category = "Misc"):
	"""Converts values from one unit to another."""
	unit_classes = [
		{
			"Nm":		1,
			"Ncm":		100,
			"Dynecm":	10000000,
			"kgm":		0.1019716,
			"kgcm":		10.19716,
			"gcm":		10197.16,
			"ozin":		141.612,
			"lbft":		0.737562,
			"lbin":		8.85074,
			"mNm":		1000,
		},
		{
			"J":		1,
			"Wh":		1 / 3600,
			"kWh":		1 / 3600 / 1000,
			"cal":		1 / 4.1868,
			"kcal":		1 / 4.1868 / 1000,
		},
		{
			"m/s":		1,
			"km/h":		3.6,
			"ft/s":		1000 / 25.4 / 12,
			"mph":		3.6 / 1.609344,
		},
		{
			"mm":		1000,
			"cm":		100,
			"m":		1,
			"km":		0.001,
			"in":		1000 / 25.4,
			"ft":		1000 / 25.4 / 12,
			"nm":		1 / 1852,
			"mile":		1 / 1609.34,
		},
		{
			"m³/h":		1,
			"ft³/min":	1 / ((0.3048 ** 3) * 60),
			"cfm":		1 / ((0.3048 ** 3) * 60),
		},
		{
			"s":		1,
			"hr":		3600,
			"d":		86400,
			"yr":		86400 * 365.25,
		},
		{
			"bar":		1,
			"mbar":		1000,
			"at":		1.0197162129779,
			"atm":		0.98692326671601,
			"Pa":		100000,
			"hPa":		1000,
			"kPa":		100,
			"MPa":		0.1,
			"psi":		14.503773773022,
			"Torr":		750.06375541921,
			"mmHg":		750.06375541921,
			"mmH2O":	10197.162129779,
		},
		{
			"kg":		1,
			"lbs":		2.20462,
		},
	]
	unknown_unit = set()
	unknown_unit.add(fromunit)
	unknown_unit.add(tounit)
	for eqclass in unit_classes:
		if (fromunit in unknown_unit) and (fromunit in eqclass):
			unknown_unit.remove(fromunit)
		if (tounit in unknown_unit) and (tounit in eqclass):
			unknown_unit.remove(tounit)
		if (fromunit in eqclass) and (tounit in eqclass):
			break
	else:
		if len(unknown_unit) == 0:
			raise Exception("No equivalence class found that contains both units '%s' and '%s'. Both units known, but in incompatible classes." % (fromunit, tounit))
		elif len(unknown_unit) == 1:
			raise Exception("No equivalence class found that contains unit '%s'." % (unknown_unit.pop()))
		else:
			raise Exception("Both units %s unknown." % (", ".join(sorted(list(unknown_unit)))))

	return value / eqclass[fromunit] * eqclass[tounit]

def dB(ratio) -> FncDescription(category = "Misc"):
	"""Converts a ratio to dB."""
	return log10(ratio) * 10

def idB(dbvalue) -> FncDescription(category = "Misc"):
	"""Converts a ratio in dB to its plain value."""
	return 10 ** (dbvalue / 10)

def sci(value, significant_digits = 3) -> FncDescription(category = "Misc"):
	"""Rounds a value and shows it in scientific notation."""
	exponent = math.floor(math.log10(value))
	divisor = 10 ** exponent
	return "%s E %d" % (str(round(value / divisor, significant_digits - 1)), exponent)

def unify(value, significant_digits = 3) -> FncDescription(category = "Misc"):
	"""Converts a value to a unified string."""
	known_powers = [
		(-18, "a"),
		(-15, "f"),
		(-12, "p"),
		(-9, "n"),
		(-6, "µ"),
		(-3, "m"),
		(0, ""),
		(3, "k"),
		(6, "M"),
		(9, "G"),
		(12, "T"),
		(15, "P"),
		(18, "E"),
	]
	value_power = math.floor(math.log10(value))
	if value_power < known_powers[0][0]:
		return "%.*e %s" % (significant_digits - 1, value / (10 ** known_powers[0][0]), known_powers[0][1])

	for (power, prefix) in known_powers:
		if (power + 3 > value_power):
			before = value_power - power + 1
			after = significant_digits - before
			fmtstr = "%%%d.%df %%s" % (before, after)
			return fmtstr % (value / 10 ** (power), prefix)

	return "%.*e %s" % (significant_digits - 1, value / (10 ** known_powers[-1][0]), known_powers[-1][1])

def linint(x1, y1, x2, y2, x) -> FncDescription(category = "Misc"):
	"""\
	Linear interpolation given two points (x1, y1) and (x2, y2) for a given
	value x."""
	t = (y2 - y1) / (x2 - x1)
	return y1 + (x - x1) * t

def area_at_ratio(area, ratio) -> FncDescription(category = "Misc"):
	"""\
	Determines the width and height of a rectangle when the area of the
	rectangle and the ratio of the two sides is given."""
	height = math.sqrt(area / ratio)
	width = area / height
	return (width, height)

def lambertW(x) -> FncDescription(category = "Numerical mathematics", aliases = [ "lambW", "lW" ]):
	"""Approximates the Lambert W function for the given value."""
	wj = 1
	while True:
		wj_new = wj - (wj * math.exp(wj) - x) / (math.exp(wj) * (1 + wj))
		if abs(wj - wj_new) < 1e-9:
			break
		wj = wj_new
	return wj_new

def testcases():
	"""Runs several self-checks on the module."""
	def approxeql(a, b):
		return abs(a - b) < 1e-6

	def ror_testcases():
		for regwidth in range(4, 16):
			for iteration in range(100):
				value = random.randint(0, (2 ** regwidth) - 1)
				rotation = random.randint(1, regwidth - 1)
				assert(ror(value, 0, regwidth) == value)
				assert(ror(value, regwidth, regwidth) == value)
				calc_value = ror(value, rotation, regwidth)
				bin_value = bin(value)[2:]
				if len(bin_value) < regwidth:
					bin_value = "0" * (regwidth - len(bin_value)) + bin_value
				expect_value = int(bin_value[-rotation:] + bin_value[:-rotation], 2)
				assert(expect_value == calc_value)

	def primesbelow_testcases():
		assert(primesbelow(100) == [2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97 ])
		assert(len(primesbelow(1000)) == 168)

	def primefactors_testcases():
		assert(primefactors(101101) == [ 7, 11, 13, 101 ])
		for i in range(10):
			x = random.randint(100000, 1000000) & ~1
			for p in primefactors(x):
				while (x % p) == 0:
					x //= p
			assert(x == 1)

	def factorlist_testcases():
		for i in range(10):
			x = random.randint(100000, 1000000) & ~1
			factorization = factorlist(x)
			assert(product(factorization) == x)

	def factor_testcases():
		for number in [ 101101, 101, 2003, 2, 2003 * 7 * 7 * 7 * 101 * 101, 738745754 ]:
			factorization = factor(number)
			composition = [ (base ** exponent) for (base, exponent) in factorization.items() ]
			product = 1
			for x in composition:
				product *= x
			assert(number == product)

	def isprime_testcases():
		for p in [ 2, 3, 5, 11, 101, 2003 ]:
			assert(isprime(p))
			assert(not isprime(2 * p))
			assert(not isprime(101 * p))
		for s in [ 561, 1105, 1729, 2465, 2821, 6601, 8911, 10585, 1234567 ]:
			assert(not isprime(s))

	def float_testcases():
		"""-"""
		maxvalue = 1000000000
		maxerr = 0
		for exponent in range(-20, 20):
			for i in range(2000):
				mantissa = randint(0, maxvalue - 1)
				value = (mantissa / maxvalue) * (2 ** exponent)
				encoded = encodedbl(value)
				decoded = decodedbl(encoded)
				err = abs(decoded - value)
				if err > maxerr:
					maxerr = err
#					print("%f -> 0x%x -> %f (err %e)" % (value, encoded, decoded, err))
				assert(maxerr < 1e-3)
		assert(maxerr < 1e-9)

	def time_testcases():
		assert(ts2timet("1970-01-01 00:00:00") == 0)
		assert(timet2ts(0) == "1970-01-01 00:00:00")
		assert(ts2timet("1970-01-02 12:34:56") == 86400 + 45296)
		assert(timet2ts(86400 + 45296) == "1970-01-02 12:34:56")

		for i in range(365):
			timet = 86400 * i
			timestr = timet2ts(timet)
			assert(timestr.endswith(" 00:00:00"))
			assert(ts2timet(timestr) == timet)

		for i in range(2000):
			timet = 753677 * i
			timestr = timet2ts(timet)
			assert(ts2timet(timestr) == timet)

	def disk_testcases():
		assert(chs2lba(255, 63, 0, 72, 21) == 4556)
		assert(chs2pos(255, 63, 0, 72, 21, 512) == 4556 * 512)

	def unit_testcases():
		conversions = [
			"1000 m = 1 km",
			"1 m = 100 cm",
			"12 in = 1 ft",
			"123.456 m = 405.03937 ft",
			"1 m = 1000 mm",
			"403.543307 ft/s = 123 m/s",
			"100 m/s = 360 km/h",
			"55 mph = 88.51392 km/h",
			"123.456 cfm = 123.456 ft³/min",
			"123 cfm = 208.978328 m³/h",
		]
		for tcstring in conversions:
			tcstring = tcstring.split(" = ")
			(lhs_coeff, lhs_unit) = tcstring[0].split()
			(rhs_coeff, rhs_unit) = tcstring[1].split()
			lhs_coeff = float(lhs_coeff)
			rhs_coeff = float(rhs_coeff)

#			print(lhs_coeff, lhs_unit, "=", rhs_coeff, rhs_unit, "=", unit(lhs_coeff, lhs_unit, rhs_unit), rhs_unit, "=", unit(rhs_coeff, rhs_unit, lhs_unit), lhs_unit)
			assert(approxeql(unit(lhs_coeff, lhs_unit, rhs_unit), rhs_coeff))
			assert(approxeql(unit(rhs_coeff, rhs_unit, lhs_unit), lhs_coeff))

	def lW_testcases():
		assert(approxeql(lambertW(0), 0))
		assert(approxeql(lambertW(0.5), 0.3517337112))
		assert(approxeql(lambertW(1), 0.5671432904))
		assert(approxeql(lambertW(10), 1.745528003))
		assert(approxeql(lambertW(100), 3.385630140))

	lW_testcases()
	unit_testcases()
	ror_testcases()
	time_testcases()
	primesbelow_testcases()
	primefactors_testcases()
	factorlist_testcases()
	factor_testcases()
	isprime_testcases()
	float_testcases()
	disk_testcases()
	return "PASS"

def __iter_functions():
	for (name, value) in dict(globals()).items():
		try:
			fncdesc = value.__annotations__["return"]
		except (AttributeError, KeyError):
			continue

		if not fncdesc.visible:
			continue

		if name in fncdesc.aliases:
			# This is an alias, ignore
			continue

		if not fncdesc.imports_satisfied:
			# Python installation does not have required packages
			continue

		yield (name, value, fncdesc)

def __get_builtins():
	builtins = [ ]
	for (name, value) in dict(globals()).items():
		typestr = value.__class__.__name__
		if typestr == "builtin_function_or_method":
			builtins.append(name)
	return builtins

if query == "":
	print("Usage: %s [Arithmetic Expression]" % (sys.argv[0]))


	def print_helptext(fnc):
		leftcols = textwrap.wrap(fnc.name + "(" + fnc.prototype + ")", width = 30, subsequent_indent = "   ")
		if fnc.description is None:
			rightcols = [ "-- no description available --" ]
		else:
			text = textwrap.dedent(fnc.description)
			if len(fnc.aliases) > 0:
				text += " Aliases: " + ", ".join(sorted(fnc.aliases))
			rightcols = textwrap.wrap(text)
		if len(rightcols) > len(leftcols):
			leftcols += [ "" ] * (len(rightcols) - len(leftcols))
		elif len(leftcols) > len(rightcols):
			rightcols += [ "" ] * (len(leftcols) - len(rightcols))
		for i in range(len(leftcols)):
			print("    %-35s %s" % (leftcols[i], rightcols[i].strip()))


	builtins = [ "log2" ] + __get_builtins()
	functions = [ ]

	for (name, value, fncdesc) in __iter_functions():
		docstr = value.__doc__
		if docstr is not None:
			docstr = textwrap.dedent(docstr)
		fncdesc.setprototype(", ".join(inspect.getfullargspec(value).args))
		fncdesc.setname(name)
		fncdesc.setdescription(docstr)
		functions.append(fncdesc)

	functions.sort()
	current_category = None
	for function in functions:
		if function.category != current_category:
			print()
			print("%s" % (function.category))
			current_category = function.category
		print_helptext(function)

	print()
	print("Other functions: %s" % (", ".join(sorted(builtins))))

	sys.exit(1)
elif query == "-l":
	for (name, value, fncdesc) in sorted(__iter_functions()):
		print(name)
	sys.exit(1)
else:
	# Define function aliases in global namespace before continuing
	for (name, value) in dict(globals()).items():
		try:
			fncdesc = value.__annotations__["return"]
		except (AttributeError, KeyError):
			continue

		for alias in fncdesc.aliases:
			globals()[alias] = value

def __evaluate(query):
	result = eval(query)
	if not query.startswith("binout("):
		print(result)

if sys.stdin.isatty():
	__evaluate(query)
else:
	for line in sys.stdin:
		line = line[:-1]
		try:
			__evaluate(query)
		except Exception as e:
			print("EXCPT at '%s': %s" % (line, str(e)))
