/* Textcore.h   All access to core text strings.
 */

/* (c) COPYRIGHT 1993-98           NOMBAS, INC.
 *                                 64 SALEM ST.
 *                                 MEDFORD, MA 02155  USA
 * 
 * ALL RIGHTS RESERVED
 * 
 * This software is the property of Nombas, Inc. and is furnished under
 * license by Nombas, Inc.; this software may be used only in accordance
 * with the terms of said license.  This copyright notice may not be removed,
 * modified or obliterated without the prior written permission of Nombas, Inc.
 * 
 * This software is a Trade Secret of Nombas, Inc.
 * 
 * This software may not be copied, transmitted, provided to or otherwise made
 * available to any other person, company, corporation or other entity except
 * as specified in the terms of said license.
 * 
 * No right, title, ownership or other interest in the software is hereby
 * granted or transferred.
 * 
 * The information contained herein is subject to change without notice and
 * should not be construed as a commitment by Nombas, Inc.
 */

#ifdef _TEXTCORE_CPP
#  ifndef _TEXTCORE_H
#     error MUST INCLUDE TEXTCORE.H BEFORE DEFINING _TEXTCORE_CPP, \
            AND THEN AGAIN AFTER DEFINING _TEXTCORE_CPP
#  endif
#  undef _TEXTCORE_H
#endif

#ifndef _TEXTCORE_H
#define _TEXTCORE_H

#if defined(_TEXTCORE_CPP)
#  undef   TC_RESOURCE
#  if !defined(JSE_SHORT_RESOURCE) || (0==JSE_SHORT_RESOURCE)
#     define TC_RESOURCE(ID,SHORT,DETAILS) UNISTR(SHORT) UNISTR(DETAILS),
#  else
#     define TC_RESOURCE(ID,SHORT,DETAILS) UNISTR(SHORT),
#  endif
#  undef   TC_TEXT_STRING
#  define  TC_TEXT_STRING(ID,STRING) \
              CONST_DATA(jsechar) textcore##ID##[] = UNISTR(STRING);
#else
#  define  TC_RESOURCE(ID,SHORT,DETAILS) textcore##ID##,
#  undef   TC_TEXT_STRING
#  define  TC_TEXT_STRING(ID,STRING) \
              extern CONST_DATA(jsechar) textcore##ID##[];
#endif


#if defined(_TEXTCORE_CPP)
CONST_DATA(jsechar *) textcoreStrings[TEXTCORE_ID_COUNT] = {
   UNISTR("Resource String Not Found."),
#else

enum textcoreID {
   RESOURCE_STRING_NOT_FOUND = 0,
#endif

/********* This is the basic concept behind the error number scheme. ********
 *
 *  0xxx:  Internal CENVI errors.
 *   00xx:  Finding source stuff.
 *   01xx:  Binding errors
 *
 *  1xxx:  Language errors
 *   10xx:  Preprocessor errors
 *   11xx:  Parsing errors
 *   12xx:  Parse error; missing a piece of a loop or something.
 *   13xx:  Misc parsing errors.
 *   14xx:  Function declaration stuff.
 *   15xx:  Expression evaluation errors
 *   16xx:  Data type errors
 *   17xx:  Math errors
 *
 *  2xxx: API errors
 *
 *  5xxx:  Library errors
 *   50xx:  Parameter types
 *   51xx:  Misc lib function errors
 *
 *  6xxx:  OS specific library error messages
 *   60xx:  Multi-os problems.
 *    609x:  Environment variable stuff.
 *   61xx:  NT
 *   62xx:  OS/2
 *   63xx:  Windows 3.1
 *   64xx:  DOS
 *   65xx:  Unix
 *
 *  7xxx:  CGI
 *
 *  8xxx:  Misc.
 *   81xx: Security
 *
 *  9xxx:  Debug version messages
 *   90xx:  Memory stuff
 *   91xx:  Unimplemented hooks
 *   92xx:  We're confused.
 *   99xx:  I haven't a clue what these mean
 *
 ***********************************************************************/


#  if (0!=JSE_COMPILER)
      TC_RESOURCE(BLOCK_TOKEN_MISSING,"block","")
      TC_RESOURCE(FUNCTION_CALL_TOKEN_MISSING,"function call","")
      TC_RESOURCE(GROUPING_TOKEN_MISSING,"grouping","")
      TC_RESOURCE(ARRAY_TOKEN_MISSING,"array","")
      TC_RESOURCE(CONDITIONAL_TOKEN_MISSING,"conditional","")
#  endif

   /* Internal ScriptEase errors. */
#  if defined(JSE_TOOLKIT_APPSOURCE) && (0!=JSE_TOOLKIT_APPSOURCE)
      TC_RESOURCE(UNABLE_TO_OPEN_SOURCE_FILE,"0001",
                  ": Unable to open source file \"%s\" for reading.")
#  endif

   /*TC_RESOURCE(BAD_STATEMENT_CODE_TYPE,"0002",
     ": Cannot evaluate code type %04X (\"%s\") here.")*/
   /*RESOURCE(INSUFFICIENT_MEMORY,"0003",
     ": Insufficient Memory to continue operation.")*/
#  if defined(CHECK_FOR_VALID_USER_KEYS)
      TC_RESOURCE(INVALID_ACCESS_KEY,"0010",": Invalid Access Key \"%s\".")
#  endif

   /*TC_RESOURCE(NYI,"0051",": (INTERNAL) Not yet implemented.")*/

   /* Problems with User's code */
#  if (defined(JSE_DEFINE) && (0!=JSE_DEFINE)) \
    || (defined(JSE_INCLUDE) && (0!=JSE_INCLUDE)) \
    || (defined(JSE_LINK) && (0!=JSE_LINK))
      TC_RESOURCE(UNRECOGNIZED_PREPROCESSOR_DIRECTIVE,"1001",
                  ": Unrecognized Preprocessor Directive \"%s\".")
#  endif

#  if (0!=JSE_COMPILER)
      TC_RESOURCE(NO_BEGIN_COMMENT,"1002",
         ": '*/' found without '/*'. probable error: comments don't nest.")
      TC_RESOURCE(END_COMMENT_NOT_FOUND,"1003",
         ": End-comment (\"*/\") never found.")
#  endif

#  if ( defined(JSE_INCLUDE) && (0!=JSE_INCLUDE) ) \
   || ( defined(JSE_LINK) && (0!=JSE_LINK) )
      TC_RESOURCE(MISSING_INCLINK_NAME_QUOTE,"1005",
         ": \"%c\" not found for #%s directive.")
#  endif

#  if defined(JSE_TOKENDST) && (0!=JSE_TOKENDST)
      /*TC_RESOURCE(TOKEN_READ_FAILURE,"1007",
        ": Failure parsing tokenized jse.")*/
#  endif


#  if defined(JSE_LINK) && (0!=JSE_LINK)
      TC_RESOURCE(LINK_LIBRARY_LOAD_FAILED, "1008",
         ": External Link Library \"%s\" Failed to Load.")
      TC_RESOURCE(LINK_LIBRARY_NOT_EXISTS, "1009",
         ": External Link Library \"%s\" Does Not Exist.")
      TC_RESOURCE(LINK_LIBRARY_FUNC_NOT_EXIST, "1010",
         ": \"%s\" Is not a valid Extension Link Library.")
      TC_RESOURCE(LINK_LIBRARY_BAD_VERSION,"1011",
         ": Incorrect Version (%d) of External Link Library \"%s\". %d is required.")
#  endif


#  if defined(JSE_CONDITIONAL_COMPILE) && (0!=JSE_CONDITIONAL_COMPILE)
      TC_RESOURCE(MUST_APPEAR_WITHIN_CONDITIONAL_COMPILATION,"1013",
         ": #%s must appear within conditional compilation (#if...)")
      TC_RESOURCE(ENDIF_NOT_FOUND,"1014",": #endif not found")
#  endif

   TC_RESOURCE(CANNOT_PROCESS_BETWEEN_QUOTES,"1107",
      ": Invalid character data between <%c> characters.")
   TC_RESOURCE(FUNC_OR_VAR_NOT_DECLARED,"1212",
      ": \"%s\" variable or function has not been declared.")
   TC_RESOURCE(BAD_DELETE_VAR,"1217",
      ": delete can only delete object members.")
   TC_RESOURCE(FUNCTION_NAME_NOT_FOUND,"1403",
      ": Could not locate function \"%s\".")
   TC_RESOURCE(FUNCPARAM_NOT_PASSED,"1404",
      ": Expected parameter %d has not been passed to function \"%s\".")
   TC_RESOURCE(NOT_FUNCTION_VARIABLE,"1406",
      ": Variable %sis not a function type.")
   TC_RESOURCE(CANNOT_COMPARE_VAR_AGAINST_OBJECT,"1514",
      ": Cannot compare variable against object.")
   TC_RESOURCE(CANNOT_COMPARE_VAR_AGAINST_STRING,"1515",
      ": Cannot compare variable against string type.")
   TC_RESOURCE(CANNOT_ASSIGN_NONNUMERIC_TO_ARRAY_ELEMENT,"1516",
      ": Cannot assign non-numeric to element of string or buffer.")
   TC_RESOURCE(IS_NAN,"1519",
       ": math operation failed due to NaN value or undefined variable.")
   TC_RESOURCE(NOT_OBJECT,"1520",
      ": new operator function must only return objects.")
   /* TC_RESOURCE(VARIABLE_IS_NOT_OBJECT,"1603",
    * ": Operator (\".\" or \"[\") is valid only on objects and C-type arrays.")
    */
   TC_RESOURCE(VAR_TYPE_UNKNOWN,"1607",": Variable \"%s\" is undefined.")
   TC_RESOURCE(NO_DEFAULT_VALUE,"1614",
      ": Object %sdoesn't have a valueOf() or toString() method.")
   TC_RESOURCE(CANNOT_CONVERT_OBJECT,"1615",
      ": Object failed to return a primitive from call to 'defaultvalue' method.")
   TC_RESOURCE(CANNOT_CONVERT_TO_OBJECT,"1616",
      ": Undefined and Null types cannot be converted to an object.")
   TC_RESOURCE(CANNOT_ADD_ARRAYS,"1701",": Cannot add arrays.")
   TC_RESOURCE(CANNOT_DIVIDE_BY_ZERO,"1702",": Cannot divide by zero.")
   TC_RESOURCE(CAN_ONLY_SUBTRACT_SAME_ARRAY,"1703",
      ": Array differences only allowed for offsets into the same array.")
   TC_RESOURCE(INVALID_PARAMETER_COUNT,"5001",
      ": Invalid parameter count %d passed to function \"%s\".")
#  if defined(JSE_SECUREJSE) && (0!=JSE_SECUREJSE)
      TC_RESOURCE(NO_APPROVAL_FROM_SECURITY_GUARD,"8100",
         ": Security function did not approve call.")
      TC_RESOURCE(NO_SECURITY_GUARD_FUNC,"8102",
         ": No Security! %s() not available. Call not approved.")
#  endif

#  if ( 0 < JSE_API_ASSERTLEVEL )
      TC_RESOURCE(PRINTERROR_FUNC_REQUIRED,"2000",
         ": PrintErrorFunc is required on jseExternalLinkparameters.")
#  endif
   TC_RESOURCE(VARNEEDED_PARAM_ERROR,"5002",
       ": Error with parameter%s %sin function \"%s\":\nType:%s. Expected:%s")
   TC_RESOURCE(PARAM_TYPE_UNDEFINED,"undefined","")
   TC_RESOURCE(PARAM_TYPE_NULL,"null","")
   TC_RESOURCE(PARAM_TYPE_BOOLEAN,"boolean","")
   TC_RESOURCE(PARAM_TYPE_OBJECT,"object","")
   TC_RESOURCE(PARAM_TYPE_STRING,"string","")
#  if defined(JSE_TYPE_BUFFER) && (0!=JSE_TYPE_BUFFER)
      TC_RESOURCE(PARAM_TYPE_BUFFER,"buffer","")
#  endif
   TC_RESOURCE(PARAM_TYPE_NUMBER,"number","")
   TC_RESOURCE(PARAM_TYPE_INT,"integer","")
   TC_RESOURCE(PARAM_TYPE_BYTE,"byte","")
   TC_RESOURCE(PARAM_TYPE_FUNCTION_OBJECT,"function object","")
   TC_RESOURCE(PARAM_TYPE_OR," or ","")
#  ifndef NDEBUG
      /*TC_RESOURCE(UNKNOWN_VARNEEDED,"0004",
        ": Don't recognize var need type %04X.")*/
      TC_RESOURCE(UNKNOWN_FUNCATTRIBUTE,"9203",
         ": FuncAttribute type %04X is unknown")
#  endif

      TC_RESOURCE(MUST_BE_LVALUE,"1704",
         ": Variable %smust be an Lvalue for assignment.")

#  if (0!=JSE_COMPILER)
      TC_RESOURCE(RESERVED_KEYWORD,"1015",
         ": Reserved keyword \"%s\" used as variable name")

      TC_RESOURCE(BAD_CHAR,"1016",
         ": Character code %x [%c] not recognized here.")
      
      TC_RESOURCE(TOKEN_MISSING,"1101",
         ": Unmatched %s \"%c\" in function \"%s\"; \"%c\" expected.")
      TC_RESOURCE(MISMATCHED_END_BRACE,"1102",
         ": Mismatched \"}\" in function \"%s\".")
      TC_RESOURCE(MISMATCHED_END_PAREN,"1103",
         ": No matching \"(\" for \")\" in function \"%s\".")
      TC_RESOURCE(MISMATCHED_END_BRACKET,"1104",
         ": No matching array \"[\" for \"]\" in function \"%s\".")
      TC_RESOURCE(NO_TERMINATING_QUOTE,"1105",
         ": No terminating <%c> for string %s.")
      TC_RESOURCE(EXPECT_COMMA_BETWEEN_ARRAY_INITS,"1106",
         ": Comma (,) expected between elements of array initialization.")
      TC_RESOURCE(MISPLACED_KEYWORD,"1108",": Misplaced keyword \"%s\".")

      TC_RESOURCE(MISSING_CLOSE_PAREN,"1109",": Expected ')' not found.")
      TC_RESOURCE(BAD_PRIMARY,"1110",
         ": Expecting varname, literal or '('; possible cause is missing ';'.")
      TC_RESOURCE(MISSING_CLOSE_BRACKET,"1111",
         ": Expected ']' after array index not found.")
      TC_RESOURCE(BAD_BREAK,"1112",
         ": break and continue can only appear in switch, for, while, and do loops.")
      TC_RESOURCE(BAD_IF,"1113",
         ": if statement must meet the form: if ( <expression> ) statement;")
      TC_RESOURCE(BAD_WHILE,"1114",
         ": while statement must meet the form: while ( <expression> ) statement;")
      TC_RESOURCE(BAD_DO,"1115",
         ": do statement must meet the form: do statement while ( <expression> );")
      TC_RESOURCE(BAD_SWITCH,"1116",
         ": switch statement must meet the form: switch( <expression> ) { ... }")
      TC_RESOURCE(BAD_GOTO_PLACE,"1117",
         ": gotos and labels cannot appear in with statements or\nfor..in loops.")
      TC_RESOURCE(DUPLICATE_DEFAULT,"1118",
         ": switch statement has more than one default case.")
      TC_RESOURCE(SWITCH_NEEDS_CASE,"1119",
         ": first statement in a switch must be a case: or the default:")
      /*TC_RESOURCE(TOO_MANY_PARAMS,"1120",
        ": function calls cannot have more than 32 parameters.")*/
      TC_RESOURCE(MISSING_PROPERTY_NAME,"1121",": Missing property name.")

      TC_RESOURCE(CASE_STATEMENT_WITHOUT_VALUE,"1201",
         ": CASE statement without value in function \"%s\".")
      TC_RESOURCE(NO_MATCHING_CONDITIONAL_OR_CASE,"1202",
         ": No matching conditional \"?\" or CASE for \":\" in function \"%s\".")
      TC_RESOURCE(CONDITIONAL_MISSING_COLON,"1203",
         ": Conditional missing \":\".")
      /*TC_RESOURCE(WHILE_NOT_FOUND_AFTER_DO,"1204",
        ": \"while\" not found after \"do\" statement.")*/
      TC_RESOURCE(BAD_FOR_STATEMENT,"1205",
         ": for statement must meet format \"for(init;test;increment) statement\".")
      /*TC_RESOURCE(SWITCH_NEEDS_EXPRESSION,"1206",
        ": Need expression after switch statement.")*/
      TC_RESOURCE(SWITCH_NEEDS_BRACE,"1207",
                  ": \"{\" must follow switch statement.")
      /*TC_RESOURCE(CANT_EVALUATE_CASE,"1208",
        ": Could not evaluate case expression.")*/
      TC_RESOURCE(NEED_GOTO_LABEL,"1209",": Need goto Label.")
      /*TC_RESOURCE(VARIABLE_NAME_MISSING,"1210",
        ": Need variable name following \"var\" keyword.")*/
      /*TC_RESOURCE(VARIABLE_ALREADY_DEFINED,"1211",
        ": Variable \"%s\" has already been declared.")*/

      /*TC_RESOURCE(FOR_IN_NEEDS_OBJECT,"1213",
        ": for statement can only iterate members of an object.")*/

      TC_RESOURCE(BAD_FOR_IN_STATEMENT,"1214",
         ": for statement must meet format \"for( varname in object ) statement\".")
      TC_RESOURCE(BAD_WITH_STATEMENT,"1215",
         ": with statement must meet format \"with ( obj ) statement;\".")
      TC_RESOURCE(VAR_NEEDS_VARNAME,"1216",
         ": var statement requires a variable to be declared.")
      TC_RESOURCE(EXTRA,"1218",
         ": Extra tokens found after function, likely mispelled 'function' keyword.")

      TC_RESOURCE(INVALID_PARAMETER_DECLARATION,"1401",
         ": Invalid parameter %d declaration for function: %s.")
      TC_RESOURCE(FUNCTION_IS_UNFINISHED,"1402",": Function %s is unfinished.")
      TC_RESOURCE(GOTO_LABEL_NOT_FOUND,"1405",
         ": Label \"%s\" not found to go to.")

#     if defined(JSE_FLOATING_POINT) && (0!=JSE_FLOATING_POINT)
         /*TC_RESOURCE(NOT_VALID_ON_FLOATS,"1612",
           ": Operation not valid with floating-point number.")*/
#     else
         TC_RESOURCE(NO_FLOATING_POINT,"1613",
           ": Floating point numbers are not supported in this version.")
#     endif
      /*TC_RESOURCE(PROTOTYPE_LOOPS,"1617",
        ": prototype chains must not be circular.")*/

#     if defined(JSE_SECUREJSE) && (0!=JSE_SECUREJSE)
         TC_RESOURCE(NO_SECURITY_WHILE_COMPILING,"8103",
            ": Insecure function not allowed during conditional compilation.")
#     endif


      TC_RESOURCE(UNMATCHED_CODE_CARD_PAIRS,"9900",
         ": Unmatched code card pairs in function \"%s\".")

#  endif

#if defined(_TEXTCORE_CPP)
   };

#else
   TEXTCORE_ID_COUNT
   };
#endif

   /* initial (global) function name must start with root character that cannot
    * be part of a real function name so that it's easy to distinguish
    */
   TC_TEXT_STRING(InitializationFunctionName,":Global Initialization:")
#  if defined(JSE_INCLUDE) && (0!=JSE_INCLUDE)
      TC_TEXT_STRING(IncludeDirective,"include")
#  endif
#  if defined(JSE_LINK) && (0!=JSE_LINK)
      TC_TEXT_STRING(ExtLinkDirective,"link")
#  endif
#  if defined(JSE_DEFINE) && (0!=JSE_DEFINE)
      TC_TEXT_STRING(DefineDirective,"define")
#  endif

#  if (0!=JSE_COMPILER) \
   || (defined(JSE_OPERATOR_OVERLOADING) && (0!=JSE_OPERATOR_OVERLOADING))
      TC_TEXT_STRING(DeleteKeyword, "delete")
      TC_TEXT_STRING(TypeofKeyword, "typeof")
      TC_TEXT_STRING(VoidKeyword, "void")
#  endif
#  if (0!=JSE_COMPILER)
      TC_TEXT_STRING(IfKeyword,"if")
      TC_TEXT_STRING(ElseKeyword,"else")
      TC_TEXT_STRING(SwitchKeyword,"switch")
      TC_TEXT_STRING(CaseKeyword,"case")
      TC_TEXT_STRING(DefaultKeyword,"default")
      TC_TEXT_STRING(WhileKeyword,"while")
      TC_TEXT_STRING(DoKeyword,"do")
      TC_TEXT_STRING(ForKeyword,"for")
      TC_TEXT_STRING(InKeyword, "in")
      TC_TEXT_STRING(WithKeyword, "with")
      TC_TEXT_STRING(CatchKeyword, "catch")
      TC_TEXT_STRING(ConstKeyword, "const")
      TC_TEXT_STRING(DebuggerKeyword, "debugger")
      TC_TEXT_STRING(EnumKeyword, "enum")
      TC_TEXT_STRING(ExportKeyword, "export")
      TC_TEXT_STRING(ClassKeyword, "class")
      TC_TEXT_STRING(ExtendsKeyword, "extends")
      TC_TEXT_STRING(FinallyKeyword, "finally")
      TC_TEXT_STRING(ImportKeyword, "import")
      TC_TEXT_STRING(SuperKeyword, "super")
      TC_TEXT_STRING(ThrowKeyword, "throw")
      TC_TEXT_STRING(TryKeyword, "try")
      TC_TEXT_STRING(BreakKeyword,"break")
      TC_TEXT_STRING(ContinueKeyword,"continue")
      TC_TEXT_STRING(GotoKeyword,"goto")
      TC_TEXT_STRING(ReturnKeyword,"return")
      TC_TEXT_STRING(NewKeyword,"new")
      TC_TEXT_STRING(VariableKeyword,"var")
#     if defined(JSE_C_EXTENSIONS) && (0!=JSE_C_EXTENSIONS)
         TC_TEXT_STRING(CFunctionKeyword,"cfunction")
#     endif

      TC_TEXT_STRING(vtype_bool_true,"true")
      TC_TEXT_STRING(vtype_bool_false,"false")
      
#  endif

   TC_TEXT_STRING(FunctionKeyword,"function")

   TC_TEXT_STRING(vtype_undefined,"undefined")
   TC_TEXT_STRING(vtype_null,"null")
   TC_TEXT_STRING(vtype_object,"object")
   TC_TEXT_STRING(vtype_NaN,"NaN")
   TC_TEXT_STRING(vtype_Infinity,"Infinity")

   TC_TEXT_STRING(ThisVariableName,"this")
   TC_TEXT_STRING(GlobalVariableName,"global")

   TC_TEXT_STRING(MainFunctionName,"main")
   TC_TEXT_STRING(_ArgcName,"_argc")
   TC_TEXT_STRING(_ArgvName,"_argv")

   TC_TEXT_STRING(InlineSourceCodePhonyFilename,"<inline source code>")

   TC_TEXT_STRING(Unknown,"unknown")
   TC_TEXT_STRING(ErrorNear,"Error near")

#  if defined(JSE_SECUREJSE) && (0!=JSE_SECUREJSE)
      TC_TEXT_STRING(SecurityInitFunctionName,"jseSecurityInit")
      TC_TEXT_STRING(SecurityTermFunctionName,"jseSecurityTerm")
      TC_TEXT_STRING(SecurityGuardFunctionName,"jseSecurityGuard")
#  endif

#if !defined(_TEXTCORE_CPP)

#  define textcoreGet(id) (textcoreStrings[id])

   extern CONST_DATA(jsechar *) textcoreStrings[TEXTCORE_ID_COUNT];

#endif

#endif
