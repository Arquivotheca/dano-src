%{

%}
%state T
%state CCOMMENT
%state CPPCOMMENT
%%
								{ BEGIN T; }
<T>\/\*							{ BEGIN CCOMMENT; }
<CCOMMENT>\*\/					{ BEGIN T; }
<CCOMMENT>.						{ /* eat comment */ }
<CPPCOMMENT>.					{ /* eat comment */ }
<CPPCOMMENT>[\n]				{ yy_line_no++; BEGIN T; }
