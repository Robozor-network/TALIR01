#!/usr/bin/env python3

import datetime
import time
import sys
import argparse
import numpy as np
from dateutil import parser as dateparser
from skyfield.api import load, Topos, EarthSatellite, utc


ts = load.timescale(builtin=True)
TLE_FILE = "https://celestrak.com/NORAD/elements/active.txt"


def parse_time(s):
	if s == "now":
		return ts.now()
	return ts.utc(dateparser.parse(s).replace(tzinfo=utc))


desc = '''
Search for satellite passes and prepare dishp tracking scripts. 

Times are UTC. Location is hardcoded to Svákov observatory.

'''
parser = argparse.ArgumentParser(description=desc)
parser.add_argument('since', metavar='SINCE', type=parse_time, default="now",
					nargs='?',
                    help='point in time at which to start searching for a satellite pass (default: now)')
parser.add_argument('--satname', dest='sat', default="2019-038AE",
                    help='name of satellite of interest (default: 2019-038AE)')
parser.add_argument('-n', dest='no', default=5, type=int,
                    help='number of upcoming passes to list (default: 5)')
parser.add_argument('--dishp', dest='dishp', default=False, action='store_true',
                    help='export a dishp tracking script for the first upcoming pass, redirect stdout'
                    	 + ' to save to a file')
parser.add_argument('--mock-start', dest='mock_start', metavar='START_TIME', type=parse_time,
                    help="when producing a dishp script, take the sky trajectory of the pass but shift"
                    	 " the time to simulate the pass beginning at the time point given as argument")


def pass_search(soi, at, since):
	start = since
	end = None
	maxele = 0
	tarray = ts.tt_jd(since.tt + np.arange(60*24/5)/60/24*3)
	alts, azs, _ = (soi - at).at(tarray).altaz()
	for t, alt, az in zip(tarray, alts.degrees, azs.degrees):
		if alt > maxele:
			maxele = alt
		if alt < 0:
			if maxele > 0:
				end = t
				return (start, end, maxele)
			start = t
	return None


def map_azs(azs, offset, start):
	return [(az-start-offset) % 360 + start for az in azs]


def azs_continuous(azs):
	return np.all(np.max(np.abs(np.diff(azs))) < 180)


def print_passes(soi, place, since, count=5):
	print("Next %d passes of sat '%s':" % (count, soi.name))
	print("")
	print("               Start     End        Max Ele  ")
	print("  ------------------------------------------ ")

	prevdate, prevstime, prevetime = "", "", ""
	for _ in range(count):
		pass_ = pass_search(soi, place, since)
		if pass_ is None:
			break
		start, end, maxele = pass_
		date = start.utc_strftime('%Y-%m-%d')
		stime = start.utc_strftime('%H:%M:%S')
		etime = end.utc_strftime('%H:%M:%S')
		print('   {:>10}  {:>8}  {:>8}  {:8.1f}'.format(
			date if date != prevdate else "",
			stime if stime != prevstime else "",
			etime if etime != prevetime else "",
			maxele
		))
		prevdate, prevstime, prevetime = date, stime, etime
		since = end
	print("")


def main():
	args = parser.parse_args()

	satellites = load.tle("https://celestrak.com/NORAD/elements/active.txt")
	place = Topos(49.2607, 14.6917)
	_sats_by_name = {sat.name: sat for sat in satellites.values()}
	soi = _sats_by_name[args.sat]

	if not args.dishp:
		print_passes(soi, place, args.since, args.no)
		return

	# search for a pass
	found_pass = pass_search(soi, place, args.since)
	if found_pass is not None:
		start, end, maxele = found_pass
		print("Found pass between %s and %s with maximum elevation %.1f"
			  % (start.utc_iso(''), end.utc_iso(''), maxele), file=sys.stderr)
	else:
		print("No pass found", file=sys.stderr)
		sys.exit(1)
		return

	points = []
	start, end, _ = found_pass
	relpos = soi - place
	tt = start.tt
	while tt < end.tt:
		t = ts.tt_jd(tt)
		alt, az, _ = relpos.at(t).altaz()
		if alt.degrees < 5:
			tt += 8 / 60 / 60 / 24
			continue
		else:
			points.append((t, alt.degrees, az.degrees))
			tt += 8 / 60 / 60 / 24

	ts_ = [t for t, _, _ in points]
	alts = [alt for _, alt, _ in points]
	azs = [az for _, _, az in points]
	good = True
	for az_start in [-270, -180, -90]:
		mapped = map_azs(azs, 180, az_start)
		if azs_continuous(mapped):
			good = True
			break
	if not good:
		print("No good azimuth mapping into actuator range", file=sys.stderr)
		sys.exit(1)
	print("Mapping azimuth into actuator range %d to %d" % (az_start, az_start+360), file=sys.stderr)
	if args.mock_start is not None:
		first = ts_[0]
		ts_ = [ts.tt_jd(t.tt - first.tt + args.mock_start.tt) for t in ts_]
	points = zip(ts_, alts, mapped)


	print("flush")
	first = True
	for t, alt, az in points:
		if first:
			print("move %.2f %.2f" % ((alt-5)/85*165*100, az*100))
			first = False
		print("point %d %.2f %.2f" % (t.utc_datetime().timestamp()*1e6, alt*100, az*100))
	print("waitidle")
	print("track")
	print("waitidle")
	print("move 0 0")
	print("waitidle")


if __name__ == "__main__":
	main()
