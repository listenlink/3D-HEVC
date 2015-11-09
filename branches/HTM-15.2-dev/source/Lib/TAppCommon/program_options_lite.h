/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.
 *
 * Copyright (c) 2010-2015, ITU/ISO/IEC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
 *    be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <iostream>
#include <sstream>
#include <string>
#include <list>
#include <map>
#include  "../TLibCommon/CommonDef.h" 
#if NH_MV
#include <vector>
#include <errno.h>
#include <cstring>

#ifdef WIN32
#define strdup _strdup
#endif
#endif


#ifndef __PROGRAM_OPTIONS_LITE__
#define __PROGRAM_OPTIONS_LITE__

//! \ingroup TAppCommon
//! \{


namespace df
{
  namespace program_options_lite
  {
    struct Options;

    struct ParseFailure : public std::exception
    {
      ParseFailure(std::string arg0, std::string val0) throw()
      : arg(arg0), val(val0)
      {}

      ~ParseFailure() throw() {};

      std::string arg;
      std::string val;

      const char* what() const throw() { return "Option Parse Failure"; }
    };

    struct ErrorReporter
    {
#if NH_MV
      ErrorReporter() : is_errored(0), output_on_unknow_parameter(true)  {}
#else
      ErrorReporter() : is_errored(0) {}
#endif
      virtual ~ErrorReporter() {}
      virtual std::ostream& error(const std::string& where);
      virtual std::ostream& warn(const std::string& where);
      bool is_errored;
#if NH_MV
      bool output_on_unknow_parameter;
#endif
    };

    extern ErrorReporter default_error_reporter;

    void doHelp(std::ostream& out, Options& opts, unsigned columns = 80);
    std::list<const char*> scanArgv(Options& opts, unsigned argc, const char* argv[], ErrorReporter& error_reporter = default_error_reporter);
    void setDefaults(Options& opts);
    void parseConfigFile(Options& opts, const std::string& filename, ErrorReporter& error_reporter = default_error_reporter);

    /** OptionBase: Virtual base class for storing information relating to a
     * specific option This base class describes common elements.  Type specific
     * information should be stored in a derived class. */
    struct OptionBase
    {
#if NH_MV      
      OptionBase(const std::string& name, const std::string& desc, bool duplicate = false, std::vector< int > maxdim = std::vector< int >(0) )
        : opt_string(name), opt_desc(desc), opt_duplicate(duplicate), max_dim( maxdim )
#else
      OptionBase(const std::string& name, const std::string& desc)
      : opt_string(name), opt_desc(desc)
#endif
      {};

      virtual ~OptionBase() {}

      /* parse argument arg, to obtain a value for the option */
#if NH_MV
      virtual void parse(const std::string& arg, const std::vector<int>& idcs,  ErrorReporter&) = 0;
      
      bool   checkDim( std::vector< int > dims, ErrorReporter& err )
      {     
        bool doParsing = true; 
        if ( dims.size() != max_dim.size() )
        {
            err.error(" ") << "Number of indices of `" <<  opt_string << "' not matching. Should be " << max_dim.size() << std::endl; 
            doParsing = false; 
        }

        for (size_t i = 0 ; i < dims.size(); i++ )
        {
          if ( dims[i] >= max_dim[i] )
          {
            if (err.output_on_unknow_parameter )
            {       
              err.warn(" ") << "Index " << i  << " of  " <<  opt_string << " should be less than " << max_dim[i] << std::endl;             
              doParsing = false; 
            }
          }
        }
        return doParsing; 
      }

      void   xParseVec( const std::string& arg, BoolAry1d& storage )
      {        
        char* pcNextStart = (char*) arg.data();
        char* pcEnd = pcNextStart + arg.length();

        char* pcOldStart = 0; 

        size_t iIdx = 0; 

        while (pcNextStart < pcEnd)
        {
          if ( iIdx < storage.size() )
          {
            storage[iIdx] = (strtol(pcNextStart, &pcNextStart,10) != 0);
          }
          else
          {
            storage.push_back(strtol(pcNextStart, &pcNextStart,10) != 0) ;
          }
          iIdx++; 

          if ( errno == ERANGE || (pcNextStart == pcOldStart) )
          {
            std::cerr << "Error Parsing Bools: `" << arg << "'" << std::endl;
            exit(EXIT_FAILURE);
          };   
          while( (pcNextStart < pcEnd) && ( *pcNextStart == ' ' || *pcNextStart == '\t' || *pcNextStart == '\r' ) ) pcNextStart++;  
          pcOldStart = pcNextStart;
        }
      }

      void   xParseVec( const std::string& arg, IntAry1d& storage )
      {        
        storage.clear();

        char* pcNextStart = (char*) arg.data();
        char* pcEnd = pcNextStart + arg.length();

        char* pcOldStart = 0; 

        size_t iIdx = 0; 


        while (pcNextStart < pcEnd)
        {

          if ( iIdx < storage.size() )
          {
            storage[iIdx] = (int) strtol(pcNextStart, &pcNextStart,10);
          }
          else
          {
            storage.push_back( (int) strtol(pcNextStart, &pcNextStart,10)) ;
          }
          iIdx++; 
          if ( errno == ERANGE || (pcNextStart == pcOldStart) )
          {
            std::cerr << "Error Parsing Integers: `" << arg << "'" << std::endl;
            exit(EXIT_FAILURE);
          };   
          while( (pcNextStart < pcEnd) && ( *pcNextStart == ' ' || *pcNextStart == '\t' || *pcNextStart == '\r' ) ) pcNextStart++;  
          pcOldStart = pcNextStart;
        }      
      }
#else
      virtual void parse(const std::string& arg, ErrorReporter&) = 0;
#endif
      /* set the argument to the default value */
      virtual void setDefault() = 0;

      std::string opt_string;
      std::string opt_desc;
#if NH_MV
      bool        opt_duplicate; 
      std::vector<int> max_dim;
#endif
    };

    /** Type specific option storage */
    template<typename T>
    struct Option : public OptionBase
    {
#if NH_MV
      Option(const std::string& name, T& storage, T default_val, const std::string& desc, bool duplicate = false, std::vector< int > maxdim = std::vector< int >(0) )
        : OptionBase(name, desc, duplicate, maxdim), opt_storage(storage), opt_default_val(default_val)
#else
      Option(const std::string& name, T& storage, T default_val, const std::string& desc)
      : OptionBase(name, desc), opt_storage(storage), opt_default_val(default_val)
#endif
      {}

#if NH_MV
      void parse(const std::string& arg, const std::vector<int>& idcs, ErrorReporter&);
#else
      void parse(const std::string& arg, ErrorReporter&);
#endif

      void setDefault()
      {
        opt_storage = opt_default_val;
      }

      T& opt_storage;
      T opt_default_val;
    };

    /* Generic parsing */
    template<typename T>
    inline void
#if NH_MV
    Option<T>::parse(const std::string& arg, const std::vector<int>& idcs, ErrorReporter&)
#else
    Option<T>::parse(const std::string& arg, ErrorReporter&)
#endif
    {
#if NH_MV
      assert( idcs.size() == 0 ); 
#endif
      
      std::istringstream arg_ss (arg,std::istringstream::in);
      arg_ss.exceptions(std::ios::failbit);
      try
      {
        arg_ss >> opt_storage;
      }
      catch (...)
      {
        throw ParseFailure(opt_string, arg);
      }
    }

    /* string parsing is specialized -- copy the whole string, not just the
     * first word */
    template<>
    inline void
#if NH_MV
    Option<std::string>::parse(const std::string& arg, const std::vector<int>& idcs, ErrorReporter&)
    {
      assert( idcs.size() == 0 ); 
      opt_storage = arg;
    }
#else
    Option<std::string>::parse(const std::string& arg, ErrorReporter&)
    {
      opt_storage = arg;
    }
#endif

#if NH_MV    
    template<>
    inline void
      Option<char*>::parse(const std::string& arg, const std::vector<int>& idcs, ErrorReporter&)
    {
      assert( idcs.size() == 0 ); 
      opt_storage = arg.empty() ? NULL : strdup(arg.c_str()) ;
    }

    template<>    
    inline void
      Option< std::vector<double> >::parse(const std::string& arg, const std::vector< int > & idcs, ErrorReporter&)
    {
      assert( idcs.size() == 0 ); 
      char* pcNextStart = (char*) arg.data();
      char* pcEnd = pcNextStart + arg.length();

      char* pcOldStart = 0; 

      size_t iIdx = 0; 

      while (pcNextStart < pcEnd)
      {
        errno = 0; 

        if ( iIdx < opt_storage.size() )
        {
          opt_storage[iIdx] = strtod(pcNextStart, &pcNextStart);
        }
        else
        {
          opt_storage.push_back( strtod(pcNextStart, &pcNextStart)) ;
        }
        iIdx++; 

        if ( errno == ERANGE || (pcNextStart == pcOldStart) )
        {
          std::cerr << "Error Parsing Doubles: `" << arg << "'" << std::endl;
          exit(EXIT_FAILURE);    
        };   
        while( (pcNextStart < pcEnd) && ( *pcNextStart == ' ' || *pcNextStart == '\t' || *pcNextStart == '\r' ) ) pcNextStart++;  
        pcOldStart = pcNextStart; 

      }
    }


    template<>
    inline void
      Option< IntAry1d >::parse(const std::string& arg, const IntAry1d& idcs, ErrorReporter& err)
    {
      xParseVec( arg, opt_storage );
    };

    template<>
    inline void
      Option< IntAry2d >::parse(const std::string& arg, const IntAry1d& idcs, ErrorReporter&)
    {
      xParseVec( arg, opt_storage[ idcs[0] ] );
    };

    template<>
    inline void
      Option< IntAry3d >::parse(const std::string& arg, const IntAry1d& idcs, ErrorReporter&)
    {
      xParseVec ( arg, opt_storage[ idcs[0] ][ idcs[1] ] );
    };

    template<>
    inline void
    Option< std::vector< std::vector<double> > >::parse(const std::string& arg, const IntAry1d& idcs, ErrorReporter&)
    {
        // xParseVec ( arg, opt_storage[ idcs[0] ] );
        char* pcNextStart = (char*) arg.data();
        char* pcEnd = pcNextStart + arg.length();

        char* pcOldStart = 0; 

        size_t iIdx = 0; 

        while (pcNextStart < pcEnd)
        {
            errno = 0; 

            if ( iIdx < opt_storage[idcs[0]].size() )
            {
                opt_storage[idcs[0]][iIdx] = strtod(pcNextStart, &pcNextStart);
            }
            else
            {
                opt_storage[idcs[0]].push_back( strtod(pcNextStart, &pcNextStart)) ;
            }
            iIdx++; 

            if ( errno == ERANGE || (pcNextStart == pcOldStart) )
            {
                std::cerr << "Error Parsing Doubles: `" << arg << "'" << std::endl;
                exit(EXIT_FAILURE);    
            };   
            while( (pcNextStart < pcEnd) && ( *pcNextStart == ' ' || *pcNextStart == '\t' || *pcNextStart == '\r' ) ) pcNextStart++;  
            pcOldStart = pcNextStart; 
        }
    }


    template<>
    inline void
      Option< StringAry1d  >::parse(const std::string& arg, const std::vector<int>& idcs, ErrorReporter& err )
    {

      opt_storage[ idcs[ 0 ] ] = arg;
    };

    template<>
    inline void
      Option< StringAry2d  >::parse(const std::string& arg, const std::vector<int>& idcs, ErrorReporter& err )
    {

      opt_storage[idcs[0]][idcs[1]] = arg;
    };


    template<>
    inline void
      Option< std::vector< char*>  >::parse(const std::string& arg, const std::vector<int>& idcs, ErrorReporter& err )
    {
      
      opt_storage[ idcs[ 0 ] ] = arg.empty() ? NULL : strdup(arg.c_str()) ;
    };

    template<>
    inline void
      Option< BoolAry1d >::parse(const std::string& arg, const std::vector<int>& idcs, ErrorReporter& err)
    {      
      xParseVec( arg, opt_storage );
    };

    template<>
    inline void
      Option< BoolAry2d >::parse(const std::string& arg, const IntAry1d& idcs, ErrorReporter& err)
    {     
      xParseVec( arg, opt_storage[ idcs[0] ] );
    };

    template<>
    inline void
      Option< BoolAry3d >::parse(const std::string& arg, const IntAry1d& idcs, ErrorReporter& err )
    {     
      xParseVec( arg, opt_storage[idcs[0]][idcs[1]] );
    };
#endif
    /** Option class for argument handling using a user provided function */
    struct OptionFunc : public OptionBase
    {
      typedef void (Func)(Options&, const std::string&, ErrorReporter&);

      OptionFunc(const std::string& name, Options& parent_, Func *func_, const std::string& desc)
      : OptionBase(name, desc), parent(parent_), func(func_)
      {}

#if NH_MV
      void parse(const std::string& arg, const std::vector<int>& idcs, ErrorReporter& error_reporter)
#else
      void parse(const std::string& arg, ErrorReporter& error_reporter)
#endif
      {
        func(parent, arg, error_reporter);
      }

      void setDefault()
      {
        return;
      }

    private:
      Options& parent;
      Func* func;
    };

    class OptionSpecific;
    struct Options
    {
      ~Options();

      OptionSpecific addOptions();

      struct Names
      {
        Names() : opt(0) {};
        ~Names()
        {
          if (opt)
          {
            delete opt;
          }
        }
        std::list<std::string> opt_long;
        std::list<std::string> opt_short;
        OptionBase* opt;
      };

      void addOption(OptionBase *opt);

      typedef std::list<Names*> NamesPtrList;
      NamesPtrList opt_list;

      typedef std::map<std::string, NamesPtrList> NamesMap;
      NamesMap opt_long_map;
      NamesMap opt_short_map;
    };

    /* Class with templated overloaded operator(), for use by Options::addOptions() */
    class OptionSpecific
    {
    public:
      OptionSpecific(Options& parent_) : parent(parent_) {}

      /**
       * Add option described by name to the parent Options list,
       *   with storage for the option's value
       *   with default_val as the default value
       *   with desc as an optional help description
       */
      template<typename T>
      OptionSpecific&
      operator()(const std::string& name, T& storage, T default_val, const std::string& desc = "")
      {
        parent.addOption(new Option<T>(name, storage, default_val, desc));
        return *this;
      }

#if NH_MV
      template<typename T>
      OptionSpecific&
        operator()(const std::string& name, std::vector<T>& storage, T default_val, unsigned uiMaxNum, const std::string& desc = "" )
      {
        std::vector<T> defVal;
        defVal.resize( uiMaxNum, default_val ); 
        std::vector< int > maxSize; 
        maxSize.push_back( uiMaxNum ); 
        parent.addOption(new Option< std::vector<T> >( name, storage, defVal, desc, false, maxSize ));

        return *this;
      }

      template<typename T>
      OptionSpecific&
        operator()(const std::string& name, std::vector< std::vector<T> >& storage, T default_val, unsigned uiMaxNumDim1, unsigned uiMaxNumDim2, const std::string& desc = "" )
      {
        std::vector< std::vector<T> > defVal;
        defVal.resize(uiMaxNumDim1);
        for ( unsigned int idxDim1 = 0; idxDim1 < uiMaxNumDim1; idxDim1++ )
        {
          defVal[ idxDim1 ].resize(uiMaxNumDim2, default_val );          
        }

        std::vector< int > maxSize; 
        maxSize.push_back( uiMaxNumDim1 ); 
        maxSize.push_back( uiMaxNumDim2 ); 

        parent.addOption(new Option< std::vector< std::vector<T> > >( name, storage, defVal, desc, false, maxSize ));
        return *this;
      }
#endif
      /**
       * Add option described by name to the parent Options list,
       *   with desc as an optional help description
       * instead of storing the value somewhere, a function of type
       * OptionFunc::Func is called.  It is upto this function to correctly
       * handle evaluating the option's value.
       */
      OptionSpecific&
      operator()(const std::string& name, OptionFunc::Func *func, const std::string& desc = "")
      {
        parent.addOption(new OptionFunc(name, parent, func, desc));
        return *this;
      }
    private:
      Options& parent;
    };

  } /* namespace: program_options_lite */
} /* namespace: df */

//! \}

#endif
