#ifndef ROBODOC_H_HEADER_INCLUDED
#define ROBODOC_H_HEADER_INCLUDED

typedef struct ASRobodocState
{

#define ASRS_InsideSection			(0x01<<0)	  
#define ASRS_VarlistSubsection		(0x01<<1)	  
#define ASRS_FormalPara				(0x01<<2)	  
#define ASRS_TitleAdded				(0x01<<3)	  

	ASFlagType flags ;
 	const char *source ;
	int len ;
	int curr ;	   

	xml_elem_t* doc ;
	xml_elem_t* curr_section ;
	xml_elem_t* curr_subsection ;

	int	last_robodoc_id ;
}ASRobodocState;

/* supported remap types ( see ROBODOC docs ): 
 * c -- Header for a class.
 * d -- Header for a constant (from define).
 * f -- Header for a function.
 * h -- Header for a module in a project.
 * m -- Header for a method.
 * s -- Header for a structure.
 * t -- Header for a types.
 * u -- Header for a unittest.
 * v -- Header for a variable.
 * * -- Generic header for every thing else.
 */
/* supported subsection headers :
 * 	NAME -- Item name plus a short description.
 *  COPYRIGHT -- Who own the copyright : "(c) <year>-<year> by <company/person>"
 *  SYNOPSIS, USAGE -- How to use it.
 *  FUNCTION, DESCRIPTION, PURPOSE -- What does it do.
 *  AUTHOR -- Who wrote it.
 *  CREATION DATE -- When did the work start.
 *  MODIFICATION HISTORY, HISTORY -- Who has done which changes and when.
 *  INPUTS, ARGUMENTS, OPTIONS, PARAMETERS, SWITCHES -- What can we feed into it.
 *  OUTPUT, SIDE EFFECTS -- What output is made.
 *  RESULT, RETURN VALUE -- What do we get returned.
 *  EXAMPLE -- A clear example of the items use.
 *  NOTES -- Any annotations
 *  DIAGNOSTICS -- Diagnostic output
 *  WARNINGS, ERRORS -- Warning and error-messages.
 *  BUGS -- Known bugs.
 *  TODO, IDEAS -- What to implement next and ideas.
 *  PORTABILITY -- Where does it come from, where will it work.
 *  SEE ALSO -- References to other functions, man pages, other documentation.
 *  METHODS, NEW METHODS -- OOP methods.
 *  ATTRIBUTES, NEW ATTRIBUTES -- OOP attributes
 *  TAGS -- Tag-item description.
 *  COMMANDS -- Command description.
 *  DERIVED FROM -- OOP super class.
 *  DERIVED BY -- OOP sub class.
 *  USES, CHILDREN -- What modules are used by this one.
 *  USED BY, PARENTS -- Which modules do use this one.
 *  SOURCE -- Source code inclusion. 
 **/

typedef enum {
 	ROBODOC_NAME_ID = 0,
	ROBODOC_COPYRIGHT_ID,
	ROBODOC_SYNOPSIS_ID,
 	ROBODOC_USAGE_ID,
 	ROBODOC_FUNCTION_ID,
	ROBODOC_DESCRIPTION_ID,
	ROBODOC_PURPOSE_ID,
	ROBODOC_AUTHOR_ID,
	ROBODOC_CREATION_DATE_ID,
	ROBODOC_MODIFICATION_HISTORY_ID,
	ROBODOC_HISTORY_ID,
 	ROBODOC_INPUTS_ID,
	ROBODOC_ARGUMENTS_ID,
	ROBODOC_OPTIONS_ID,
	ROBODOC_PARAMETERS_ID,
	ROBODOC_SWITCHES_ID,
 	ROBODOC_OUTPUT_ID,
	ROBODOC_SIDE_EFFECTS_ID,
 	ROBODOC_RESULT_ID,
	ROBODOC_RETURN_VALUE_ID,
 	ROBODOC_EXAMPLE_ID,
 	ROBODOC_NOTES_ID,
 	ROBODOC_DIAGNOSTICS_ID,
 	ROBODOC_WARNINGS_ID,
	ROBODOC_ERRORS_ID,
 	ROBODOC_BUGS_ID,
 	ROBODOC_TODO_ID,
	ROBODOC_IDEAS_ID,
 	ROBODOC_PORTABILITY_ID,
 	ROBODOC_SEE_ALSO_ID,
 	ROBODOC_METHODS_ID,
	ROBODOC_NEW_METHODS_ID,
 	ROBODOC_ATTRIBUTES_ID,
	ROBODOC_NEW_ATTRIBUTES_ID,
 	ROBODOC_TAGS_ID,
 	ROBODOC_COMMANDS_ID,
 	ROBODOC_DERIVED_FROM_ID,
 	ROBODOC_DERIVED_BY_ID,
 	ROBODOC_USES_ID,
	ROBODOC_CHILDREN_ID,
 	ROBODOC_USED_BY_ID,
	ROBODOC_PARENTS_ID,
 	ROBODOC_SOURCE_ID,
	ROBODOC_SUPPORTED_IDs                          
}SupportedRoboDocTagIDs;

void gen_code_doc( const char *source_dir, const char *dest_dir, 
			  const char *file, 
			  const char *display_name,
			  const char *display_purpose,
			  ASDocType doc_type );

#endif

