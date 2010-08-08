/*
 * SQLModule.cpp
 *
 * This file is part of the SQLHandler a C++ library to add relational
 * database access to the OPeNDAP Hyrax back-end server.
 * Copyright (C) 2010  Carlo Cancellieri <ccancellieri@hotmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301  USA
 *
 * Created on: 24/Jun/2010
 * Author: carlo cancellieri
 *
 */

#include <iostream>

using std::endl ;

#include "BESRequestHandlerList.h"
#include "BESDebug.h"
#include "BESResponseHandlerList.h"
#include "BESResponseNames.h"

#include "SQLModule.h"
#include "SQLResponseNames.h"
#include "DEM/SQLCheckPoint.h"

#include "unixODBC/ODBCPlugin.h"

#include <BESDapService.h>
#include <BESContainerStorageList.h>
#include <BESContainerStorageCatalog.h>
#include <BESCatalogDirectory.h>
#include <BESCatalogList.h>

SQLContainerStorage *SQLModule::cs=NULL;

SQLContainerFactory SQLModule::cf=SQLContainerFactory();

SQLRequestHandler SQLModule::rh=SQLRequestHandler(SQL_NAME);


void
SQLModule::initialize( const string &modname )
{
    BESDEBUG( SQL_NAME, "Initializing SQL Module "
		       << SQL_NAME << endl ) ;
    /**
	 *  initialize check point reading
	 *  configuration file.
	 */
    SQLCheckPoint::init();

#if 0
/**HYSTORY
 * OLFS uses the same symbolic_name for each container
 * so all container are named catalogContainer which means
 * that only the first selected container is loaded and will
 * be changed only if it is marked as !isUpToDate
 *
 * Anyway the StorageContainer is used as temporary storage
 * for SQLContainer(s).
 *
 * The naming policy is actually defined by
 * SQLContainerFactory::getName(...)
 */
#endif
    BESDEBUG( SQL_NAME, "    adding " << _SQLH_STORAGE <<
    		"sql storage container" << endl ) ;

    if (!BESContainerStorageList::TheList()->ref_persistence(_SQLH_STORAGE))
    {
    	/**
		 * @note: this implementation of SQLRequestHandler uses
		 * SQLContainerStorageVolatile if you want to implement
		 * your own persistence you can use the
		 * SQLContainer::setPersistence(SQLContainerStorage*)
		 * to change it on the fly.
		 * You could also set it to NULL to make SQLContainerFactory
		 * to rebuild the SQLContainer each time it is requested.
		 */
    	cs=new SQLContainerStorageVolatile(_SQLH_STORAGE);

    	BESContainerStorage *bcs=NULL;
    	if (bcs=dynamic_cast<BESContainerStorage*>(cs))
    	{
			if (!BESContainerStorageList::TheList()->add_persistence(bcs))
			{
				throw BESInternalFatalError(
					"SQLModule: Unable to add ContainerStorage",
					__FILE__,__LINE__);
			}
		}
    	else {
    		BESDEBUG(SQL_NAME,
    			"Unable to add the ContainerStorage it is not "
    			"a BESContainerStorage instance");
    	}
    }
    else {
    	if (!(cs=dynamic_cast<SQLContainerStorage*>(
    			BESContainerStorageList::TheList()->find_persistence(_SQLH_STORAGE))))

    		/**
			 * @note: this implementation of SQLRequestHandler uses
			 * SQLContainerStorageVolatile if you want to implement
			 * your own persistence you can use the
			 * SQLContainer::setPersistence(SQLContainerStorage*)
			 * to change it on the fly.
			 * You could also set it to NULL to make SQLContainerFactory
			 * to rebuild the SQLContainer each time it is requested.
			 */
    		cs=new SQLContainerStorageVolatile(_SQLH_STORAGE);
    }
    /**
     *  Set the ContainerStorage
     *  @see SQLContainerFactory
     *  @note: SQLContainerStorage can work without persistence
     *  (if sql_cs is NULL)
     *  @note: default select_container function is
     *  used, you can change it using setSelector()
     */
	cf.setPersistence(cs);

	BESDEBUG( SQL_NAME, "    adding " << _SQLH_CATALOG << " catalog" << endl ) ;
	if( !BESCatalogList::TheCatalogList()->ref_catalog( _SQLH_CATALOG ) )
	{
		BESCatalogList::TheCatalogList()->
			add_catalog( new BESCatalogDirectory( _SQLH_CATALOG ) ) ;
	}
	else
	{
		BESDEBUG( SQL_NAME, "    catalog already exists, skipping" << endl ) ;
	}

	BESDEBUG( SQL_NAME, "    adding catalog container storage"
		<< _SQLH_CATALOG << endl ) ;
	if( !BESContainerStorageList::TheList()->ref_persistence( _SQLH_CATALOG ) )
	{
		BESContainerStorageCatalog *csc =
			new BESContainerStorageCatalog( _SQLH_CATALOG ) ;
		BESContainerStorageList::TheList()->add_persistence( csc ) ;
	}
	else
	{
		BESDEBUG( SQL_NAME, "    storage already exists, skipping" << endl ) ;
	}
	/**
	 * adding BASE sql handler which will wrap all the
	 * sql requests
	 */
	BESDEBUG( SQL_NAME, "    adding "<<SQL_NAME<< " request handler" << endl ) ;
    BESRequestHandlerList::TheList()->add_handler(SQL_NAME, &rh);

    BESDEBUG( SQL_NAME, modname << " handles dap services" << endl ) ;
    BESDapService::handle_dap_service( modname ) ;

    /**
	 * Add default ODBC SQL plugin to RequestHandlerList
	 */
    BESDEBUG( ODBC_NAME, "    using "<< ODBC_NAME
        		<<" as DEFAULT SQL request handler" << endl ) ;
    SQLRequestHandler::add_sql_handler(ODBC_NAME, new ODBCPlugin(ODBC_NAME));

    BESDEBUG( modname, "    adding sql debug context" << endl ) ;
    BESDebug::Register( SQL_NAME ) ;
    BESDEBUG( modname, "    adding odbc debug context" << endl ) ;
    BESDebug::Register( ODBC_NAME ) ;

    // INIT_END
    BESDEBUG( modname, "Done Initializing SQL Module "
		       << modname << endl ) ;
}

void
SQLModule::terminate( const string &modname )
{
    BESDEBUG( modname, "Cleaning SQL module " << modname << endl ) ;

    BESDEBUG( modname,"    removing " << ODBC_NAME << " sql plugin handler" << endl ) ;
    SQLRequestHandler::remove_sql_handler(ODBC_NAME);

    BESDEBUG( modname, "    removing " << SQL_NAME << " request handler" << endl ) ;
    BESRequestHandlerList::TheList()->remove_handler( SQL_NAME ) ;

	// READ COMMENTS IN INITIALIZE
    BESDEBUG( SQL_NAME, "    removing sql storage container"
    		<< _SQLH_STORAGE << endl ) ;
    if (cs && !BESContainerStorageList::TheList()->deref_persistence( _SQLH_STORAGE ))
		delete cs;
	cs=0;

    BESDEBUG( SQL_NAME, "    removing catalog container storage"
			<< _SQLH_CATALOG << endl ) ;
	BESContainerStorageList::TheList()->deref_persistence( _SQLH_CATALOG ) ;

	BESDEBUG( SQL_NAME, "    removing " << _SQLH_CATALOG << " catalog" << endl) ;
	BESCatalogList::TheCatalogList()->deref_catalog( _SQLH_CATALOG ) ;

    // If new commands are needed, then let's declare this once here. If
    // not, then you can remove this line.
    //string cmd_name ;

    // TERM_END
    BESDEBUG( modname, "Done Cleaning SQL module "
		       << modname << endl ) ;
}

extern "C"
{
    BESAbstractModule *maker()
    {
	return new SQLModule ;
    }
}

void
SQLModule::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "SQLModule::dump - ("
			     << (void *)this << ")" << endl ;
}

