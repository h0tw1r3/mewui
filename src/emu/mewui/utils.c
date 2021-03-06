// license:BSD-3-Clause
// copyright-holders:Dankan1890
/***************************************************************************

    mewui/utils.c

    Internal MEWUI user interface.

***************************************************************************/

#include "emu.h"
#include "mewui/utils.h"
#include "mewui/inifile.h"
#include "sound/samples.h"
#include "audit.h"

// Years index
UINT16 c_year::actual = 0;
std::vector<std::string> c_year::ui;

// Manufacturers index
UINT16 c_mnfct::actual = 0;
std::vector<std::string> c_mnfct::ui;

// Main filters
UINT16 main_filters::actual = 0;
const char *main_filters::text[] = { "All", "Available", "Unavailable", "Working", "Not Mechanical", "Category", "Favorites", "BIOS",
                                     "Originals", "Clones", "Not Working", "Mechanical", "Manufacturers", "Years", "Support Save",
                                     "Not Support Save", "CHD", "No CHD", "Use Samples", "Not Use Samples", "Stereo", "Vertical",
                                     "Horizontal", "Screen Type", "Custom" };
size_t main_filters::length = ARRAY_LENGTH(main_filters::text);

// Software filters
UINT16 sw_filters::actual = 0;
const char *sw_filters::text[] = { "All", "Available", "Unavailable", "Originals", "Clones", "Years", "Publishers", "Supported",
                                   "Partial Supported", "Unsupported", "Region", "Device Type", "Software List", "Custom" };
size_t sw_filters::length = ARRAY_LENGTH(sw_filters::text);

// Screens
UINT16 c_screen::actual = 0;
const char *c_screen::text[] = { "<none>", "Raster", "Vector", "LCD" };
size_t c_screen::length = ARRAY_LENGTH(c_screen::text);

// UME
UINT16 ume_filters::actual = 0;
const char *ume_filters::text[] = { "ALL", "ARCADES", "SYSTEMS" };
size_t ume_filters::length = ARRAY_LENGTH(ume_filters::text);

// Globals
UINT8 mewui_globals::rpanel = 0;
UINT8 mewui_globals::curimage_view = 0;
UINT8 mewui_globals::curdats_view = 0;
UINT8 mewui_globals::cur_sw_dats_view = 0;
bool mewui_globals::switch_image = false;
bool mewui_globals::default_image = true;
bool mewui_globals::reset = false;
bool mewui_globals::redraw_icon = false;
int mewui_globals::visible_main_lines = 0;
int mewui_globals::visible_sw_lines = 0;
UINT16 mewui_globals::panels_status = 0;

// Custom filter
UINT16 custfltr::main = 0;
UINT16 custfltr::numother = 0;
UINT16 custfltr::other[MAX_CUST_FILTER];
UINT16 custfltr::mnfct[MAX_CUST_FILTER];
UINT16 custfltr::year[MAX_CUST_FILTER];
UINT16 custfltr::screen[MAX_CUST_FILTER];

// Custom filter
UINT16 sw_custfltr::main = 0;
UINT16 sw_custfltr::numother = 0;
UINT16 sw_custfltr::other[MAX_CUST_FILTER];
UINT16 sw_custfltr::mnfct[MAX_CUST_FILTER];
UINT16 sw_custfltr::year[MAX_CUST_FILTER];
UINT16 sw_custfltr::region[MAX_CUST_FILTER];
UINT16 sw_custfltr::type[MAX_CUST_FILTER];
UINT16 sw_custfltr::list[MAX_CUST_FILTER];

//-------------------------------------------------
//  search a substring with even partial matching
//-------------------------------------------------

int fuzzy_substring(const char *needle, const char *haystack)
{
	const int nlen = strlen(needle);
	const int hlen = strlen(haystack);

	if (hlen == 0) return nlen;
	if (nlen == 0) return hlen;

	std::string s_needle(needle);
	std::string s_haystack(haystack);

	strmakelower(s_needle);
	strmakelower(s_haystack);

	if (s_needle == s_haystack)
		return 0;
	if (s_haystack.find(s_needle) != std::string::npos)
		return 0;

	int *row1 = global_alloc_array_clear(int, hlen + 2);
	int *row2 = global_alloc_array_clear(int, hlen + 2);

	for (int i = 0; i < nlen; ++i)
	{
		row2[0] = i + 1;
		for (int j = 0; j < hlen; ++j)
		{
			int cost = (s_needle[i] == s_haystack[j]) ? 0 : 1;
			row2[j + 1] = MIN(row1[j + 1] + 1, MIN(row2[j] + 1, row1[j] + cost));
		}

		int *tmp = row1;
		row1 = row2;
		row2 = tmp;
	}

	int *first, *smallest;
	first = smallest = row1;
	int *last = row1 + hlen;

	while (++first != last)
		if (*first < *smallest)
			smallest = first;

	int rv = *smallest;
	global_free_array(row1);
	global_free_array(row2);

	return rv;
}
//-------------------------------------------------
//  search a substring with even partial matching
//-------------------------------------------------

int fuzzy_substring2(const char *needle, const char *haystack)
{
	std::string s1(needle);
	std::string s2(haystack);
	strmakelower(s1);
	strmakelower(s2);
	const size_t m(s1.size());
	const size_t n(s2.size());

	if (m == 0)
		return n;
	if (n == 0)
		return m;

	if (s1 == s2)
		return 0;
	size_t it = s2.find(s1);
	if (it != std::string::npos)
		return it;

	size_t *costs = global_alloc_array(size_t, n + 1);
	for(size_t k = 0; k <= n; ++k)
		costs[k] = k;
	size_t i = 0;
	for (std::string::const_iterator it1 = s1.begin(); it1 != s1.end(); ++it1, ++i)
	{
		costs[0] = i+1;
		size_t corner = i;
		size_t j = 0;
		for (std::string::const_iterator it2 = s2.begin(); it2 != s2.end(); ++it2, ++j)
		{
			size_t upper = costs[j+1];
			if (*it1 == *it2)
				costs[j+1] = corner;
			else
			{
				size_t t(upper < corner ? upper : corner);
				costs[j+1] = (costs[j] < t ? costs[j] : t) + 1;
			}
			corner = upper;
		}
	}

	size_t result = costs[n];
	global_free_array(costs);
	return result;
}

//-------------------------------------------------
//  set manufacturers
//-------------------------------------------------

void c_mnfct::set(const char *str)
{
	std::string name = getname(str);
	if (std::find(ui.begin(), ui.end(), name) != ui.end())
		return;

	ui.push_back(name);
}

std::string c_mnfct::getname(const char *str)
{
	std::string name(str);
	size_t found = name.find("(");

	if (found != std::string::npos)
		return (name.substr(0, found - 1));
	else
		return name;
}

//-------------------------------------------------
//  set years
//-------------------------------------------------

void c_year::set(const char *str)
{
	std::string name(str);
	if (std::find(ui.begin(), ui.end(), name) != ui.end())
		return;

	ui.push_back(name);
}

std::ifstream &clean_getline(std::ifstream &is, std::string &line)
{
	if (std::getline(is, line)) {
		size_t epos = line.find_last_not_of("\r\n");
		if ( std::string::npos != epos )
			line.erase(epos+1);
		else
			line.clear();
	}
	return is;
}
