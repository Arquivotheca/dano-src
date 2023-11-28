
\let\'=\indexdummyfont
\let\^=\indexdummyfont
\let\~=\indexdummyfont
\let\==\indexdummyfont
\let\b=\indexdummyfont
\let\c=\indexdummyfont
\let\d=\indexdummyfont
\let\u=\indexdummyfont
\let\v=\indexdummyfont
\let\H=\indexdummyfont
% Take care of the plain tex special European modified letters.
\def\oe{oe}%
\def\ae{ae}%
\def\aa{aa}%
\def\OE{OE}%
\def\AE{AE}%
\def\AA{AA}%
\def\o{o}%
\def\O{O}%
\def\l{l}%
\def\L{L}%
\def\ss{ss}%
\let\w=\indexdummyfont
\let\t=\indexdummyfont
\let\r=\indexdummyfont
\let\i=\indexdummyfont
\let\b=\indexdummyfont
\let\emph=\indexdummyfont
\let\strong=\indexdummyfont
\let\cite=\indexdummyfont
\let\sc=\indexdummyfont
%Don't no-op \tt, since it isn't a user-level command
% and is used in the definitions of the active chars like <, >, |...
%\let\tt=\indexdummyfont
\let\tclose=\indexdummyfont
\let\code=\indexdummyfont
\let\file=\indexdummyfont
\let\samp=\indexdummyfont
\let\kbd=\indexdummyfont
\let\key=\indexdummyfont
\let\var=\indexdummyfont
\let\TeX=\indexdummytex
\let\dots=\indexdummydots
}

% To define \realbackslash, we must make \ not be an escape.
% We must first make another character (@) an escape
% so we do not become unable to do a definition.

{\catcode`\@=0 \catcode`\\=\other
@gdef@realbackslash{\}}

\let\indexbackslash=0  %overridden during \printindex.

\def\doind #1#2{%
{\count10=\lastpenalty %
{\indexdummies % Must do this here, since \bf, etc expand at this stage
\escapechar=`\\%
{\let\folio=0% Expand all macros now EXCEPT \folio
\def\rawbackslashxx{\indexbackslash}% \indexbackslash isn't defined now
% so it will be output as is; and it will print as backslash in the indx.
%
% Now process the index-string once, with all font commands turned off,
% to get the string to sort the index by.
{\indexnofonts
\xdef\temp1{#2}%
}%
% Now produce the complete index entry.  We process the index-string again,
% this time with font commands expanded, to get what to print in the index.
\edef\temp{%
\write \csname#1indfile\endcsname{%
\realbackslash entry {\temp1}{\folio}{#2}}}%
\temp }%
}\penalty\count10}}

\def\dosubind #1#2#3{%
{\count10=\lastpenalty %
{\indexdummies % Must do this here, since \bf, etc expand at this stage
\escapechar=`\\%
{\let\folio=0%
\def\rawbackslashxx{\indexbackslash}%
%
% Now process the index-string once, with all font commands turned off,
% to get the string to sort the index by.
{\indexnofonts
\xdef\temp1{#2 #3}%
}%
% Now produce the complete index entry.  We process the index-string again,
% this time with font commands expanded, to get what to print in the index.
\edef\temp{%
\write \csname#1indfile\endcsname{%
\realbackslash entry {\temp1}{\folio}{#2}{#3}}}%
\temp }%
}\penalty\count10}}

% The index entry written in the file actually looks like
%  \entry {sortstring}{page}{topic}
% or
%  \entry {sortstring}{page}{topic}{subtopic}
% The texindex program reads in these files and writes files
% containing these kinds of lines:
%  \initial {c}
%     before the first topic whose initial is c
%  \entry {topic}{pagelist}
%     for a topic that is used without subtopics
%  \primary {topic}
%     for the beginning of a topic that is used with subtopics
%  \secondary {subtopic}{pagelist}
%     for each subtopic.

% Define the user-accessible indexing commands
% @findex, @vindex, @kindex, @cindex.

\def\findex {\fnindex}
\def\kindex {\kyindex}
\def\cindex {\cpindex}
\def\vindex {\vrindex}
\def\tindex {\tpindex}
\def\pindex {\pgindex}

\def\cindexsub {\begingroup\obeylines\cindexsub}
{\obeylines %
\gdef\cindexsub "#1" #2^^M{\endgroup %
\dosubind{cp}{#2}{#1}}}

% Define the macros used in formatting output of the sorted index material.

% This is what you call to cause a particular index to get printed.
% Write
% @unnumbered Function Index
% @printindex fn

\def\printindex{\parsearg\doprintindex}

\def\doprintindex#1{%
  \tex
  \dobreak \chapheadingskip {10000}
  \catcode`\%=\other\catcode`\&=\other\catcode`\#=\other
  \catcode`\$=\other
  \catcode`\~=\other
  \indexbreaks
  %
  % The following don't help, since the chars were translated
  % when the raw index was written, and their fonts were discarded
  % due to \indexnofonts.
  %\catcode`\"=\active
  %\catcode`\^=\active
  %\catcode`\_=\active
  %\catcode`\|=\active
  %\catcode`\<=\active
  %\catcode`\>=\active
  % %
  \def\indexbackslash{\rawbackslashxx}
  \indexfonts\rm \tolerance=9500 \advance\baselineskip -1pt
  \begindoublecolumns
  %
  % See if the index file exists and is nonempty.
  \openin 1 \jobname.#1s
  \ifeof 1
    % \enddoublecolumns gets confused if there is no text in the index,
    % and it loses the chapter title and the aux file entries for the
    % index.  The easiest way to prevent this problem is to make sure
    % there is some text.
    (Index is nonexistent)
    \else
    %
    % If the index file exists but is empty, then \openin leaves \ifeof
    % false.  We have to make TeX try to read something from the file, so
    % it can discover if there is anything in it.
    \read 1 to \temp
    \ifeof 1
      (Index is empty)
    \else
      \input \jobname.#1s
    \fi
  \fi
  \closein 1
  \enddoublecolumns
  \Etex
}

% These macros are used by the sorted index file itself.
% Change them to control the appearance of the index.

% Same as \bigskipamount except no shrink.
% \balancecolumns gets confused if there is any shrink.
\newskip\initialskipamount \initialskipamount 12pt plus4pt

\def\initial #1{%
{\let\tentt=\sectt \let\tt=\sectt \let\sf=\sectt
\ifdim\lastskip<\initialskipamount
\removelastskip \penalty-200 \vskip \initialskipamount\fi
\line{\secbf#1\hfill}\kern 2pt\penalty10000}}

% This typesets a paragraph consisting of #1, dot leaders, and then #2
% flush to the right margin.  It is used for index and table of contents
% entries.  The paragraph is indented by \leftskip.
%
\def\entry #1#2{\begingroup
  %
  % Start a new paragraph if necessary, so our assignments below can't
  % affect previous text.
  \par
  %
  % Do not fill out the last line with white space.
  \parfillskip = 0in
  %
  % No extra space above this paragraph.
  \parskip = 0in
  %
  % Do not prefer a separate line ending with a hyphen to fewer lines.
  \finalhyphendemerits = 0
  %
  % \hangindent is only relevant when the entry text and page number
  % don't both fit on one line.  In that case, bob suggests starting the
  % dots pretty far over on the line.  Unfortunately, a large
  % indentation looks wrong when the entry text itself is broken across
  % lines.  So we use a small indentation and put up with long leaders.
  %
  % \hangafter is reset to 1 (which is the value we want) at the start
  % of each paragraph, so we need not do anything with that.
  \hangindent=2em
  %
  % When the entry text needs to be broken, just fill out the first line
  % with blank space.
  \rightskip = 0pt plus1fil
  %
  % Start a ``paragraph'' for the index entry so the line breaking
  % parameters we've set above will have an effect.
  \noindent
  %
  % Insert the text of the index entry.  TeX will do line-breaking on it.
  #1%
  % The following is kluged to not output a line of dots in the index if
  % there are no page numbers.  The next person who breaks this will be
  % cursed by a Unix daemon.
  \def\tempa{{\rm }}%
  \def\tempb{#2}%
  \edef\tempc{\tempa}%
  \edef\tempd{\tempb}%
  \ifx\tempc\tempd\ \else%
    %
    % If we must, put the page number on a line of its own, and fill out
    % this line with blank space.  (The \hfil is overwhelmed with the
    % fill leaders glue in \indexdotfill if the page number does fit.)
    \hfil\penalty50
    \null\nobreak\indexdotfill % Have leaders before the page number.
    %
    % The `\ ' here is removed by the implicit \unskip that TeX does as
    % part of (the primitive) \par.  Without it, a spurious underfull
    % \hbox ensues.
    \ #2% The page number ends the paragraph.
  \fi%
  \par
\endgroup}

% Like \dotfill except takes at least 1 em.
\def\indexdotfill{\cleaders
  \hbox{$\mathsurround=0pt \mkern1.5mu ${\it .}$ \mkern1.5mu$}\hskip 1em plus 1fill}

\def\primary #1{\line{#1\hfil}}

\newskip\secondaryindent \secondaryindent=0.5cm

\def\secondary #1#2{
{\parfillskip=0in \parskip=0in
\hangindent =1in \hangafter=1
\noindent\hskip\secondaryindent\hbox{#1}\indexdotfill #2\par
}}

%% Define two-column mode, which is used in indexes.
%% Adapted from the TeXbook, page 416.
\catcode `\@=11

\newbox\partialpage

\newdimen\doublecolumnhsize

\def\begindoublecolumns{\begingroup
  % Grab any single-column material above us.
  \output = {\global\setbox\partialpage
    =\vbox{\unvbox255\kern -\topskip \kern \baselineskip}}%
  \eject
  %
  % Now switch to the double-column output routine.
  \output={\doublecolumnout}%
  %
  % Change the page size parameters.  We could do this once outside this
  % routine, in each of @smallbook, @afourpaper, and the default 8.5x11
  % format, but then we repeat the same computation.  Repeating a couple
  % of assignments once per index is clearly meaningless for the
  % execution time, so we may as well do it once.
  %
  % First we halve the line length, less a little for the gutter between
  % the columns.  We compute the gutter based on the line length, so it
  % changes automatically with the paper format.  The magic constant
  % below is chosen so that the gutter has the same value (well, +- <
  % 1pt) as it did when we hard-coded it.
  %
  % We put the result in a separate register, \doublecolumhsize, so we
  % can restore it in \pagesofar, after \hsize itself has (potentially)
  % been clobbered.
  %
  \doublecolumnhsize = \hsize
    \advance\doublecolumnhsize by -.04154\hsize
    \divide\doublecolumnhsize by 2
  \hsize = \doublecolumnhsize
  %
  % Double the \vsize as well.  (We don't need a separate register here,
  % since nobody clobbers \vsize.)
  \vsize = 2\vsize
  \doublecolumnpagegoal
}

\def\enddoublecolumns{\eject \endgroup \pagegoal=\vsize \unvbox\partialpage}

\def\doublecolumnsplit{\splittopskip=\topskip \splitmaxdepth=\maxdepth
  \global\dimen@=\pageheight \global\advance\dimen@ by-\ht\partialpage
  \global\setbox1=\vsplit255 to\dimen@ \global\setbox0=\vbox{\unvbox1}
  \global\setbox3=\vsplit255 to\dimen@ \global\setbox2=\vbox{\unvbox3}
  \ifdim\ht0>\dimen@ \setbox255=\vbox{\unvbox0\unvbox2} \global\setbox255=\copy5 \fi
  \ifdim\ht2>\dimen@ \setbox255=\vbox{\unvbox0\unvbox2} \global\setbox255=\copy5 \fi
}
\def\doublecolumnpagegoal{%
  \dimen@=\vsize \advance\dimen@ by-2\ht\partialpage \global\pagegoal=\dimen@
}
\def\pagesofar{\unvbox\partialpage %
  \hsize=\doublecolumnhsize % have to restore this since output routine
  \wd0=\hsize \wd2=\hsize \hbox to\pagewidth{\box0\hfil\box2}}
\def\doublecolumnout{%
  \setbox5=\copy255
  {\vbadness=10000 \doublecolumnsplit}
  \ifvbox255
    \setbox0=\vtop to\dimen@{\unvbox0}
    \setbox2=\vtop to\dimen@{\unvbox2}
    \onepageout\pagesofar \unvbox255 \penalty\outputpenalty
  \else
    \setbox0=\vbox{\unvbox5}
    \ifvbox0
      \dimen@=\ht0 \advance\dimen@ by\topskip \advance\dimen@ by-\baselineskip
      \divide\dimen@ by2 \splittopskip=\topskip \splitmaxdepth=\maxdepth
      {\vbadness=10000
	\loop \global\setbox5=\copy0
          \setbox1=\vsplit5 to\dimen@
          \setbox3=\vsplit5 to\dimen@
          \ifvbox5 \global\advance\dimen@ by1pt \repeat
        \setbox0=\vbox to\dimen@{\unvbox1}
        \setbox2=\vbox to\dimen@{\unvbox3}
        \global\setbox\partialpage=\vbox{\pagesofar}
        \doublecolumnpagegoal
      }
    \fi
  \fi
}

\catcode `\@=\other
\message{sectioning,}
% Define chapters, sections, etc.

\newcount \chapno
\newcount \secno        \secno=0
\newcount \subsecno     \subsecno=0
\newcount \subsubsecno  \subsubsecno=0

% This counter is funny since it counts through charcodes of letters A, B, ...
\newcount \appendixno  \appendixno = `\@
\def\appendixletter{\char\the\appendixno}

\newwrite \contentsfile
% This is called from \setfilename.
\def\opencontents{\openout \contentsfi