/* Copyright (C) 2014-2017
   Elettra - Sincrotrone Trieste S.C.p.A.
   Strada Statale 14 - km 163,5 in AREA Science Park
   34149 Basovizza, Trieste, Italy.

   This file is part of libhdb++.

   libhdb++ is free software: you can redistribute it and/or modify
   it under the terms of the Lesser GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   libhdb++ is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the Lesser
   GNU General Public License for more details.

   You should have received a copy of the Lesser GNU General Public License
   along with libhdb++.  If not, see <http://www.gnu.org/licenses/>. */

#include "LibHdb++.h"
#include "LibHdb++Config.h"
#ifndef _TG_WINDOWS_
#include <dlfcn.h>
#endif

HdbClient::HdbClient(vector<string> configuration)
{
	cout << "Starting version: " 
		<< VERSION_MAJOR << "." << VERSION_MINOR << "." << VERSION_REVISION << ":" 
		<< BUILD_TIME << endl;

	map<string,string> db_conf;
	string_vector2map(configuration,"=",&db_conf);
	string libname;

	try
	{
		libname = db_conf.at("libname");
	}
	catch(const std::out_of_range& e)
	{
		cout << __func__<< ": " << "Configuration parsing error looking for key 'libname='" << endl;
		exit(1);
	}

#ifdef _TG_WINDOWS_
	if ((hLib = LoadLibrary(libname.c_str())))
	{
		if(getDBFactory_t* create_factory = (getDBFactory_t*)GetProcAddress(hLib, "getDBFactory"))
#else
	if ((hLib = dlopen(libname.c_str(), RTLD_NOW/*|RTLD_GLOBAL*/)))
	{
		if (getDBFactory_t* create_factory = (getDBFactory_t*)dlsym(hLib, "getDBFactory"))
#endif
		{
			db_factory = create_factory();
			db = db_factory->create_db(configuration);
			if(db == NULL)
			{
				cout << __func__<<": Error creating db" << endl;
				exit(1);
			}
		}
		else
		{
			cout << __func__<<": Error loading symbol getDBFactory from library " << libname << endl;
			exit(1);
		}
	}
	else
	{
		db = NULL;
	#ifdef _TG_WINDOWS_
		char *errinfo;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM, nullptr, 
			GetLastError(), 
			MAKELANGID(/*LANG_NEUTRAL*/LANG_ENGLISH, SUBLANG_DEFAULT), 
			(LPTSTR)&errinfo, 0, nullptr);
		cout << __func__<<": Error loading library " << libname << ". Error report: " << errinfo << endl;
		LocalFree(errinfo);
	#else
		cout << __func__<<": Error loading library " << libname << ". Error report: " << dlerror() << endl;
	#endif
		exit(1);
	}
}

void HdbClient::insert_Attr(Tango::EventData *data, HdbEventDataType ev_data_type)
{
	db->insert_Attr(data, ev_data_type);
}

void HdbClient::insert_param_Attr(Tango::AttrConfEventData *data, HdbEventDataType ev_data_type)
{
	db->insert_param_Attr(data, ev_data_type);
}

void HdbClient::configure_Attr(string name, int type/*DEV_DOUBLE, DEV_STRING, ..*/, int format/*SCALAR, SPECTRUM, ..*/, int write_type/*READ, READ_WRITE, ..*/, unsigned int ttl/*hours, 0=infinity*/)
{
	db->configure_Attr(name, type, format, write_type, ttl);
}

void HdbClient::updateTTL_Attr(string name, unsigned int ttl/*hours, 0=infinity*/)
{
	db->updateTTL_Attr(name, ttl);
}

void HdbClient::event_Attr(string name, unsigned char event)
{
	db->event_Attr(name, event);
}

HdbClient::~HdbClient()
{
	delete db;
	delete db_factory;
#ifdef _TG_WINDOWS_
	FreeLibrary(hLib);
#else
	dlclose(hLib);
#endif
}

void HdbClient::string_explode(string str, string separator, vector<string>* results)
{
	string::size_type found;

	found = str.find_first_of(separator);
	while(found != string::npos)
	{
		if(found > 0)
		{
			results->push_back(str.substr(0,found));
		}
		str = str.substr(found+1);
		found = str.find_first_of(separator);
	}
	if(str.length() > 0)
	{
		results->push_back(str);
	}
}

void HdbClient::string_vector2map(vector<string> str, string separator, map<string,string>* results)
{
	for(vector<string>::iterator it=str.begin(); it != str.end(); it++)
	{
		string::size_type found_eq;
		found_eq = it->find_first_of(separator);
		if(found_eq != string::npos && found_eq > 0)
			results->insert(make_pair(it->substr(0,found_eq),it->substr(found_eq+1)));
	}
}
