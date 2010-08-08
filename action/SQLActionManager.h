/*
 * SQLActionManager.h
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
 *  Created on: 22/giu/2010
 *      Author: Carlo Cancellieri
 */

#ifndef SQLACTIONMANAGER_H_
#define SQLACTIONMANAGER_H_

#include "action/SQLActionFactory.h"

#include <BESError.h>
#include <BESInternalError.h>
#include <BESInternalFatalError.h>

/**
 * @brief A manager for ActionFactory objects.
 *
 * This class defines two static methods which are
 * used to run Actions produced by an ActionFactory,
 * manage their output, and control thrown exceptions.
 * <br>The EntryPoint is doActions which is used to manage
 * the Actions output with two main operations:
 * - merge() which 'merge' the output of each Action of
 * 		an ActionList with the previous
 * - join() which 'join' the output resulting from merge()
 * 		building the output of the doActions function.
 * <br>The 'merge' and 'join' function are defined as:
 * 	_JOIN * join(_JOIN *,_MERGE *)
 * 	_MERGE * merge(_MERGE *, OUT_TYPE *)
 * <br>Those functions can be passed at doActions() function
 * to change data management on the fly.
 * <br>The second main static method (private) is TryNex
 * which control the errors thrown by Actions casting
 * to SQL counterpart which are recognizable from the
 * SQLHandler as SQLAction specific errors.
 */
template <	class OUT_TYPE=void, // output type of Actions
			class _MERGE=OUT_TYPE, //
			class _JOIN=_MERGE>
class SQLActionManager {
protected:
	/**
	 * @brief Default constructor
	 */
	SQLActionManager(){};

	/**
	 * @brief Can use _MERGE status partials to build a _JOIN final status.
	 * Join is the operation that build an object starting
	 * from the _merge_status calculated from the current ActionList
	 * (corresponding to a Code).
	 * This function should delete actual pointer if
	 * not used.
	 * @return the previous _JOIN status (NULL)
	 * <br> NOTE: do not delete actual.
	 *
	 */
	typedef _JOIN * (*JOIN)(_JOIN *,_MERGE *);
	static _JOIN * join(_JOIN * previous, _MERGE * actual){
		return previous;
		// no modification to previous
// commented out to permit void _MERGE==void
//		smart::Delete::smartDelete<_MERGE*>(actual);
	}

	/**
	 * @brief use Action output to build a _MERGE status.
	 * Merge is the operation that build an object resulting
	 * from the execution of the Actions of the ActionList
	 * corresponding to the actual Code.
	 * This function should delete actual pointer if
	 * not used.
	 * @return the previous _MERGE status (NULL)
	 * <br> NOTE: do not delete actual.
	 */
	typedef _MERGE * (*MERGE)(_MERGE *, OUT_TYPE *);
	static _MERGE * merge(_MERGE * previous, OUT_TYPE * actual){
		return previous;
		// no modification to previous
// commented out to permit OUT_TYPE == void
//		smart::Delete::smartDelete<OUT_TYPE*>(actual);
	}
public:



	/**
	 * @brief Execute actions generated by the ActionFactory
	 * and stores results into the JOIN status variable.
	 * @parameter CODE_TYPE the type of the code used by the
	 * passed ActionFactory
	 * @parameter ARGS_TYPE the type of the argument used by
	 * the actions executed by this ActionFactory
	 * @parameter OUT_TYPE the output type of executed action
	 * @param _af The ActionFactory to be executed
	 * @param merge_f The function used to merge OUT_TYPE in
	 * MERGE_TYPE
	 * @param free_merged A boolean indicating if merged
	 * elements (OUT_TYPE) must be deleted (true) or not
	 * (false)
	 * @param join_f The function used to join MERGE_TYPE in
	 * JOIN_TYPE
	 * @param multiple_args A boolean indicating if getArgs
	 * should be called for each Action of the ActionList
	 * (true) or only one time (false) as for getCode. This
	 * mean that ALL the action in the selected ActionList
	 * will run using same argument.
	 * @throws @see tryNext()
	 * @see join(), merge()
	 *
	 */
	template <class CODE_TYPE, class ARGS_TYPE>
	static _JOIN* doActions(SQLActionFactory<CODE_TYPE,ARGS_TYPE,OUT_TYPE> &_af,
			MERGE merge_f=&merge,
			JOIN join_f=&join,
			bool multiple_args=true)
	{
		if (!merge_f)
			merge_f=&merge;
		if (!join_f)
			join_f=&join;
TESTDEBUG(SQL_NAME,"ActionManager: Starting actions"<<std::endl);
		CODE_TYPE *code=NULL;
		_JOIN *_join=NULL;
		// do actions
		do {
			_MERGE *_merge=NULL;
#if __TESTS__==1
			BESDEBUG(SQL_NAME,"ActionManager: Getting code"<<std::endl);
#endif
			// get next code
			if ((code=_af.getCode())){
				// do actions for that code

				SQLActionList<ARGS_TYPE,OUT_TYPE> &actions=
					_af.getActions(code);
#if __TESTS__==1
				BESDEBUG(SQL_NAME,"ActionManager: ActionList size: "<<(actions).getSize()<<endl);
#endif
				// Used to initialize first time 'args'
				bool first_time=true;

				/**
				 * NOTE '*args' could be a null
				 * argument so no check is performed
				 */
				ARGS_TYPE *args=NULL;
				while(actions.hasNext()){
					// multiple_args || first_time == true?
					if (multiple_args)
						args=_af.getArgs(code);
					else if (first_time){
						first_time=false;
						args=_af.getArgs(code);
					}

					// tryNext action and merge results
					_merge=merge_f(_merge,tryNext(actions,args));

#if __TESTS__==1
					if (_merge)
						BESDEBUG(SQL_NAME,"ActionManager: MERGE!NULL"<<std::endl);
					else
						BESDEBUG(SQL_NAME,"ActionManager: MERGEisNULL"<<std::endl);
#endif

				}
			} // if code
			else // ERROR_TYPE *code==NULL
				break;

			// join resulting _merge_status
			_join=join_f(_join,_merge);

#if __TESTS__==1
			if (_join)
				BESDEBUG(SQL_NAME,"ActionManager: JOIN!NULL"<<std::endl);
			else
				BESDEBUG(SQL_NAME,"ActionManager: JOINisNULL"<<std::endl);
#endif

			// resetting _merge_status
			_merge=NULL;


		// while NOT stop
		}while (!_af.stop(code));//(!stop(code));//

		return _join;
	} // doActions

private:
	/**
	 * @brief Use the code to find
	 * the matching list of action to do
	 * then execute each SQLAction in
	 * the list.
	 * @param the action list to use
	 * @param the args pointer to pass to the action
	 * @return the output pointer of the action to run
	 * @throws SQLInternalFatalError,
	 * SQLInternalError, BESInternalFatalError
	 */
	template <class ARGS_TYPE>
	static OUT_TYPE * tryNext(SQLActionList<ARGS_TYPE,OUT_TYPE> &actions,
		ARGS_TYPE *args)
			throw (SQLInternalFatalError,
					SQLInternalError,
					BESInternalFatalError) {
		try {
			return actions.doNext(args);
		}
		catch (SQLInternalError &ie){
			// this error shouldn't be filtered!
			throw;
		}
		catch (SQLInternalFatalError &ife){
			// this error shouldn't be filtered!
			BESDEBUG(SQL_NAME,ife.get_message());
			throw;
		}
		/**
		 * Each not mapped error throws an exception
		 * so default operation is DEBUG only.
		 */
		catch (BESInternalError &e){
			/*
			 * This is commented out to avoid the use
			 * of CONTINUE action.
			 * If you make this error throwable here
			 * you have to map ALL of your ODBC api
			 * to a custom CONTINUE action since each
			 * time we do not find the mapped action
			 * for an error code (of TYPE ERROR_TYPE)
			 * a BESError exception is thrown
			 *
			 * throw BESError(
				"There was a problem running ACTIONs: "+e.get_message(),
					BES_INTERNAL_ERROR,__FILE__,__LINE__);

			throw SQLInternalError(
				"SQLActionManager: running ACTIONs-> "
					+e.get_message(),e.get_file(),e.get_line());
			*/
			// Generic BESError shouldn't be throw
			BESDEBUG(SQL_NAME,e.get_message());
		}
		catch (BESError &e){
			throw SQLInternalFatalError(
				"SQLActionManager: running ACTIONs-> "
					+e.get_message(),e.get_file(),e.get_line());
		}
		/*
		 * ALL other errors aren't actually filtered
		catch (...){
		}*/
		return NULL; //to avoid warning
	}

};

#endif /* SQLACTIONMANAGER_H_ */
