// This file is part of the AspectC++ compiler 'ac++'.
// Copyright (C) 1999-2004  The 'ac++' developers (see aspectc.org)
//
// This program is free software;  you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of
// the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public
// License along with this program; if not, write to the Free
// Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
// MA  02111-1307  USA

#include "PumaConfigFile.h"
#include "StdSystem.h"
#include "file.h"
#include "regex.h"
using namespace file;
using namespace regex;

//stdc++ includes
#include<fstream>
#include<sstream>

//Puma includes
#include "Puma/VerboseMgr.h"
#include "Puma/SysCall.h"

bool
PumaConfigFile::searchFile()
{
  // IF the location of the puma config file is given by an command line argument
  // check it exist
  if (_config.pumaconfig_file().empty() == false)
  {
    if (fileExists(_config.pumaconfig_file().c_str()))
    {
      return true;
    }
    else
    {
      return false;
    }
  }

  //
  // set filename of puma configuration file
  //
  if (_config.pumaconfig_tmp() == true)
  {
    //create temporary file
    _config.pumaconfig_file(Puma::SysCall::mktemp("agxx_pcfg", &_err));
  }
  else if ((_config.output_file().empty() == false)
      && (_config.nr_files() == 0))
  {
    // name of puma configuration file is specified using '-o' option
    _config.pumaconfig_file(_config.output_file());
  }
  else
  {
    // default name for puma configuration
    _config.pumaconfig_file("puma.config");
  }

  return false;
}

bool
PumaConfigFile::generateFile()
{

  VerboseMgr vm(cout, _config.verbose());
  vm << "Generating Puma configuration file" << endvm;

  // first parse c++ output
  vm++;
  vm << "Parsing output of g++ compiler" << endvm;
  if (!parseCcOutput())
  {
    return false;
  }

  // if parsing was successfull write puma config file
  vm << "Writing puma configuration file '" << _config.pumaconfig_file() << "'"
      << endvm;
  return writeFile();
}

bool
PumaConfigFile::parseCcOutput()
{

#ifdef WIN32
  const char* empty_file_name=_config.pumaconfig_file().c_str();
  ofstream empty_file(empty_file_name);

  if(! empty_file)
  {
    _err << sev_error << "Could not create " << empty_file_name << endMessage;
    return false;
  }
  empty_file.close();
#else
  const char* empty_file_name = "/dev/null";
#endif

  string config_command_str(_config.config_command());
  // check if a sepcial command for generating the out is available
  if (config_command_str.empty())
  {
    //Construct execution string for C++ Compiler
    config_command_str = "\"" + _config.cc_bin() + "\" "
        + _config.optvec().getString(
            (OptionItem::OPT_GCC | OptionItem::OPT_CONFIG))
        + " -E -dM -v -x c++ \"" + empty_file_name + "\"";
  }

  // get c compiler output
  StdSystem gcc_out(_err, _config, config_command_str);

  if (!gcc_out.execute())
  {
    return false;
  }

  // c compiler output string: whole output(stderr,stdout) of gcc
  string output(gcc_out.stderr_str());
  output.append(gcc_out.stdout_str());

  // stringstream of c compiler output string
  istringstream iss(output);

  // parse c compiler output string
  bool in_include_list = false;
  string line;
  while (getline(iss, line))
  {

    // start of list of include paths
    if (regExMatch("#include [<\"].*:.*", line.c_str()))
    {
      in_include_list = true;
    }

    // within a list of include paths
    else if (in_include_list && regExMatch("^ .*", line.c_str()))
    {

      // strip leading white space
      unsigned int strpos = line.find_first_not_of(" ");
      line = line.substr(strpos);

      // convert '//*' to '/'
      char lastchar = '0';
      for (string::iterator c = line.begin(); c != line.end(); ++c)
      {
        if (lastchar == *c && *c == '/')
        {
          c = line.erase(c);
          --c;
        }
        lastchar = *c;
      }

      // check for mac os framework directory paths
      // this is generated by clang which replaces g++ on mac os
      if (line.size() > 22 && line.substr(line.size() - 22) == " (framework directory)")
    	// remove trailing text " (framework directory)" if necessary
    	line = line.substr(0, line.size() - 22);

      // save modified string with include path
      _inc_paths.push_back(line);
    }

    // end of list of include paths
    else if (in_include_list)
    {
      in_include_list = false;
    }

    // match a '#define ...'
    else if (regExMatch("#define .*", line.c_str()))
    {

      // remove '#define '(8 characters)
      line = line.substr(8);

      // split option and argument.

      // find name of option => split string at whitespace
      string::size_type strpos = line.find(" ");
      string option = line.substr(0, strpos);

      // set empty string as default argument if no argument is defined
      string value = "";

      // find value => find a string behind the option name
      if ((strpos != string::npos)
          && (line.find_first_not_of(" ", strpos) != string::npos))
      {
        value = line.substr(strpos + 1);
      }

      _def_options.insert(opt_pair(option, value));
    }

    // match a 'Target: ...'
    else if (_target_triple.empty () && regExMatch("Target: .*", line.c_str()))
    {

      // remove 'Target: '(8 characters)
      line = line.substr(8);
      _target_triple = line;
    }
  }
  // check if string has been processed correctly
  if (!iss.eof())
  {
    _err << "Error reading gcc output string " << endMessage;
  }

  return true;
}

bool
PumaConfigFile::writeFile()
{
  // include paths need specail treatment in a cygwin environment
  bool is_cygwin = false;
  string gcc_major, gcc_minor, gcc_patchlevel;

  ofstream puma_file(_config.pumaconfig_file().c_str());

  if (!puma_file)
  {
    _err << sev_error << "Could not open " << _config.pumaconfig_file().c_str()
        << endMessage;
    return false;
  }

  puma_file << "--skip-bodies-non-prj" << endl;
  puma_file << "-D __puma" << endl;
  puma_file << "-D __STDC__" << endl;

  // Print  options
  for (map<string, string>::iterator opt = _def_options.begin();
      opt != _def_options.end(); opt++)
  {

    // print line '-D <optionname> = <value>' in puma.config
    puma_file << "-D \"" << opt->first;
    if (opt->second != "")
    {
      puma_file << "=" << _config.str_replace_all(opt->second, "\"", "\\\"");
    }
    puma_file << "\"" << endl;

    // Check gcc version
    if (opt->first == "__GNUC__")
    {
      gcc_major = opt->second;
    }
    else if (opt->first == "__GNUC_MINOR__")
    {
      gcc_minor = opt->second;
    }
    else if (opt->first == "__GNUC_PATCHLEVEL__")
    {
      gcc_patchlevel = opt->second;
    }

    // Check for cygwin
    if ((opt->first == "__CYGWIN32__") || (opt->first == "__CYGWIN__"))
    {
      is_cygwin = true;
    }

    // Check for the size_t macro
    if (opt->first == "__SIZE_TYPE__")
    {
      puma_file << "--size-type \"" << opt->second << "\"" << endl;
    }

    // Check for the ptrdiff_t macro
    if (opt->first == "__PTRDIFF_TYPE__")
    {
      puma_file << "--ptrdiff-type \"" << opt->second << "\"" << endl;
    }
  }

  // print the target triple if we know it
  if (!_target_triple.empty())
  {
	puma_file << "--target " << _target_triple << endl;
  }

  // print the --gnu option with the collected version number
  puma_file << "--gnu";
  if (gcc_major != "")
  {
    puma_file << " " << gcc_major;
    if (gcc_minor != "")
    {
      puma_file << "." << gcc_minor;
      if (gcc_patchlevel != "")
      {
        puma_file << "." << gcc_patchlevel;
      }
    }
    puma_file << endl;
  }

  // Print include statements into puma.config file, or
  // -- if we have a cygwin environment -- append it
  // to the cygpath execution string which determins
  // the correct location of the include files.
  string cygpath_exec_str = "cygpath -w ";
  for (vector<string>::iterator file = _inc_paths.begin();
      file != _inc_paths.end(); file++)
  {

    // if we have a cygwin environment use cygpath for finding the right
    // include paths.

    if (is_cygwin)
    {
      cygpath_exec_str = cygpath_exec_str + " " + *file;
    }
    else
    {
      puma_file << "--isystem \"" << *file << "\"" << endl;
    }
  }

  // print cygwin include paths into file
  if (is_cygwin)
  {
    StdSystem cygpath(_err, _config, cygpath_exec_str);
    cygpath.execute();

    istringstream iss(cygpath.stdout_str());
    string line;
    while (getline(iss, line))
    {
      puma_file << "--isystem \"" << line << "\"" << endl;
    }
  }

  puma_file.close();
  return true;
}

