on = \appendixsec
\global\let\subsection = \appendixsubsec
\global\let\subsubsection = \appendixsubsubsec
}}

\outer\def\top{\parsearg\unnumberedyyy}
\outer\def\unnumbered{\parsearg\unnumberedyyy}
\def\unnumberedyyy #1{\unnmhead0{#1}} % normally unnmhead0 calls unnumberedzzz
\def\unnumberedzzz #1{\seccheck{unnumbered}%
\secno=0 \subsecno=0 \subsubsecno=0
%
% This used to be simply \message{#1}, but TeX fully expands the
% argument to \message.  Therefore, if #1 contained @-commands, TeX
% expanded them.  For example, in `@unnumbered The @cite{Book}', TeX
% expanded @cite (which turns out to cause errors because \cite is meant
% to be executed, not expanded).
%
% Anyway, we don't want the fully-expanded definition of @cite to appear
% as a result of the \message, we just want `@cite' itself.  We use
% \the<toks register> to achieve this: TeX expands \the<toks> only once,
% simply yielding the contents of the <toks register>.
\toks0 = {#1}\message{(\the\toks0)}%
%
\unnumbchapmacro {#1}%
\gdef\thischapter{#1}\gdef\thissection{#1}%
{\chapternofonts%
\edef\temp{{\realbackslash unnumbchapentry {#1}{\noexpand\folio}}}%
\escapechar=`\\%
\write \contentsfile \temp  %
\unnumbnoderef %
\global\let\section = \unnumberedsec
\global\let\subsection = \unnumberedsubsec
\global\let\subsubsection = \unnumberedsubsubsec
}}

\outer\def\numberedsec{\parsearg\secyyy}
\def\secyyy #1{\numhead1{#1}} % normally calls seczzz
\def\seczzz #1{\seccheck{section}%
\subsecno=0 \subsubsecno=0 \global\advance \secno by 1 %
\gdef\thissection{#1}\secheading {#1}{\the\chapno}{\the\secno}%
{\chapternofonts%
\edef\temp{{\realbackslash secentry %
{#1}{\the\chapno}{\the\secno}{\noexpand\folio}}}%
\escapechar=`\\%
\write \contentsfile \temp %
\donoderef %
\penalty 10000 %
}}

\outer\def\appenixsection{\parsearg\appendixsecyyy}
\outer\def\appendixsec{\parsearg\appendixsecyyy}
\def\appendixsecyyy #1{\apphead1{#1}} % normally calls appendixsectionzzz
\def\appendixsectionzzz #1{\seccheck{appendixsection}%
\subsecno=0 \subsubsecno=0 \global\advance \secno by 1 %
\gdef\thissection{#1}\secheading {#1}{\appendixletter}{\the\secno}%
{\chapternofonts%
\edef\temp{{\realbackslash secentry %
{#1}{\appendixletter}{\the\secno}{\noexpand\folio}}}%
\escapechar=`\\%
\write \contentsfile \temp %
\appendixnoderef %
\penalty 10000 %
}}

\outer\def\unnumberedsec{\parsearg\unnumberedsecyyy}
\def\unnumberedsecyyy #1{\unnmhead1{#1}} % normally calls unnumberedseczzz
\def\unnumberedseczzz #1{\seccheck{unnumberedsec}%
\plainsecheading {#1}\gdef\thissection{#1}%
{\chapternofonts%
\edef\temp{{\realbackslash unnumbsecentry{#1}{\noexpand\folio}}}%
\escapechar=`\\%
\write \contentsfile \temp %
\unnumbnoderef %
\penalty 10000 %
}}

\outer\def\numberedsubsec{\parsearg\numberedsubsecyyy}
\def\numberedsubsecyyy #1{\numhead2{#1}} % normally calls numberedsubseczzz
\def\numberedsubseczzz #1{\seccheck{subsection}%
\gdef\thissection{#1}\subsubsecno=0 \global\advance \subsecno by 1 %
\subsecheading {#1}{\the\chapno}{\the\secno}{\the\subsecno}%
{\chapternofonts%
\edef\temp{{\realbackslash subsecentry %
{#1}{\the\chapno}{\the\secno}{\the\subsecno}{\noexpand\folio}}}%
\escapechar=`\\%
\write \contentsfile \temp %
\donoderef %
\penalty 10000 %
}}

\outer\def\appendixsubsec{\parsearg\appendixsubsecyyy}
\def\appendixsubsecyyy #1{\apphead2{#1}} % normally calls appendixsubseczzz
\def\appendixsubseczzz #1{\seccheck{appendixsubsec}%
\gdef\thissection{#1}\subsubsecno=0 \global\advance \subsecno by 1 %
\subsecheading {#1}{\appendixletter}{\the\secno}{\the\subsecno}%
{\chapternofonts%
\edef\temp{{\realbackslash subsecentry %
{#1}{\appendixletter}{\the\secno}{\the\subsecno}{\noexpand\folio}}}%
\escapechar=`\\%
\write \contentsfile \temp %
\appendixnoderef %
\penalty 10000 %
}}

\outer\def\unnumberedsubsec{\parsearg\unnumberedsubsecyyy}
\def\unnumberedsubsecyyy #1{\unnmhead2{#1}} %normally calls unnumberedsubseczzz
\def\unnumberedsubseczzz #1{\seccheck{unnumberedsubsec}%
\plainsecheading {#1}\gdef\thissection{#1}%
{\chapternofonts%
\edef\temp{{\realbackslash unnumbsubsecentry{#1}{\noexpand\folio}}}%
\escapechar=`\\%
\write \contentsfile \temp %
\unnumbnoderef %
\penalty 10000 %
}}

\outer\def\numberedsubsubsec{\parsearg\numberedsubsubsecyyy}
\def\numberedsubsubsecyyy #1{\numhead3{#1}} % normally numberedsubsubseczzz
\def\numberedsubsubseczzz #1{\seccheck{subsubsection}%
\gdef\thissection{#1}\global\advance \subsubsecno by 1 %
\subsubsecheading {#1}
  {\the\chapno}{\the\secno}{\the\subsecno}{\the\subsubsecno}%
{\chapternofonts%
\edef\temp{{\realbackslash subsubsecentry %
  {#1}
  {\the\chapno}{\the\secno}{\the\subsecno}{\the\subsubsecno}
  {\noexpand\folio}}}%
\escapechar=`\\%
\write \contentsfile \temp %
\donoderef %
\penalty 10000 %
}}

\outer\def\appendixsubsubsec{\parsearg\appendixsubsubsecyyy}
\def\appendixsubsubsecyyy #1{\apphead3{#1}} % normally appendixsubsubseczzz
\def\appendixsubsubseczzz #1{\seccheck{appendixsubsubsec}%
\gdef\thissection{#1}\global\advance \subsubsecno by 1 %
\subsubsecheading {#1}
  {\appendixletter}{\the\secno}{\the\subsecno}{\the\subsubsecno}%
{\chapternofonts%
\edef\temp{{\realbackslash subsubsecentry{#1}%
  {\appendixletter}
  {\the\secno}{\the\subsecno}{\the\subsubsecno}{\noexpand\folio}}}%
\escapechar=`\\%
\write \contentsfile \temp %
\appendixnoderef %
\penalty 10000 %
}}

\outer\def\unnumberedsubsubsec{\parsearg\unnumberedsubsubsecyyy}
\def\unnumberedsubsubsecyyy #1{\unnmhead3{#1}} %normally unnumberedsubsubseczzz
\def\unnumberedsubsubseczzz #1{\seccheck{unnumberedsubsubsec}%
\plainsecheading {#1}\gdef\thissection{#1}%
{\chapternofonts%
\edef\temp{{\realbackslash unnumbsubsubsecentry{#1}{\noexpand\folio}}}%
\escapechar=`\\%
\write \contentsfile \temp %
\unnumbnoderef %
\penalty 10000 %
}}

% These are variants which are not "outer", so they can appear in @ifinfo.
% Actually, they should now be obsolete; ordinary section commands should work.
\def\infotop{\parsearg\unnumberedzzz}
\def\infounnumbered{\parsearg\unnumberedzzz}
\def\infounnumberedsec{\parsearg\unnumberedseczzz}
\def\infounnumberedsubsec{\parsearg\unnumberedsubseczzz}
\def\infounnumberedsubsubsec{\parsearg\unnumberedsubsubseczzz}

\def\infoappendix{\parsearg\appendixzzz}
\def\infoappendixsec{\parsearg\appendixseczzz}
\def\infoappendixsubsec{\parsearg\appendixsubseczzz}
\def\infoappendixsubsubsec{\parsearg\appendixsubsubseczzz}

\def\infochapter{\parsearg\chapterzzz}
\def\infosection{\parsearg\sectionzzz}
\def\infosubsection{\parsearg\subsectionzzz}
\def\infosubsubsection{\parsearg\subsubsectionzzz}

% These macros control what the section commands do, according
% to what kind of chapter we are in (ordinary, appendix, or unnumbered).
% Define them by default for a numbered chapter.
\global\let\section = \numberedsec
\global\let\subsection = \numberedsubsec
\global\let\subsubsection = \numberedsubsubsec

% Define @majorheading, @heading and @subheading

% NOTE on use of \vbox for chapter headings, section headings, and
% such:
%	1) We use \vbox rather than the earlier \line to permit
%	   overlong headings to fold.
%	2) \hyphenpenalty is set to 10000 because hyphenation in a
%	   heading is obnoxious; this forbids it.
%       3) Likewise, headings look best if no \parindent is used, and
%          if justification is not attempted.  Hence \raggedright.


\def\majorheading{\parsearg\majorheadingzzz}
\def\majorheadingzzz #1{%
{\advance\chapheadingskip by 10pt \chapbreak }%
{\chapfonts \vbox{\hyphenpenalty=10000\tolerance=5000
                  \parindent=0pt\raggedright
                  \rm #1\hfill}}\bigskip \par\penalty 200}

\def\chapheading{\parsearg\chapheadingzzz}
\def\chapheadingzzz #1{\chapbreak %
{\chapfonts \vbox{\hyphenpenalty=10000\tolerance=5000
                  \parindent=0pt\raggedright
                  \rm #1\hfill}}\bigskip \par\penalty 200}

\def\heading{\parsearg\secheadingi}

\def\subheading{\parsearg\subsecheadingi}

\def\subsubheading{\parsearg\subsubsecheadingi}

% These macros generate a chapter, section, etc. heading only
% (including whitespace, linebreaking, etc. around it),
% given all the information in convenient, parsed form.

%%% Args are the skip and penalty (usually negative)
\def\dobreak#1#2{\par\ifdim\lastskip<#1\removelastskip\penalty#2\vskip#1\fi}

\def\setchapterstyle #1 {\csname CHAPF#1\endcsname}

%%% Define plain chapter starts, and page on/off switching for it
% Parameter controlling skip before chapter headings (if needed)

\newskip \chapheadingskip \chapheadingskip = 30pt plus 8pt minus 4pt

\def\chapbreak{\dobreak \chapheadingskip {-4000}}
\def\chappager{\par\vfill\supereject}
\def\chapoddpage{\chappager \ifodd\pageno \else \hbox to 0pt{} \chappager\fi}

\def\setchapternewpage #1 {\csname CHAPPAG#1\endcsname}

\def\CHAPPAGoff{
\global\let\pchapsepmacro=\chapbreak
\global\let\pagealignmacro=\chappager}

\def\CHAPPAGon{
\global\let\pchapsepmacro=\chappager
\global\let\pagealignmacro=\chappager
\global\def\HEADINGSon{\HEADINGSsingle}}

\def\CHAPPAGodd{
\global\let\pchapsepmacro=\chapoddpage
\global\let\pagealignmacro=\chapoddpage
\global\def\HEADINGSon{\HEADINGSdouble}}

\CHAPPAGon

\def\CHAPFplain{
\global\let\chapmacro=\chfplain
\global\let\unnumbchapmacro=\unnchfplain}

\def\chfplain #1#2{%
  \pchapsepmacro
  {%
    \chapfonts \vbox{\hyphenpenalty=10000\tolerance=5000
                     \parindent=0pt\raggedright
                     \rm #2\enspace #1}%
  }%
  \bigskip
  \penalty5000
}

\def\unnchfplain #1{%
\pchapsepmacro %
{\chapfonts \vbox{\hyphenpenalty=10000\tolerance=5000
                  \parindent=0pt\raggedright
                  \rm #1\hfill}}\bigskip \par\penalty 10000 %
}
\CHAPFplain % The default

\def\unnchfopen #1{%
\chapoddpage {\chapfonts \vbox{\hyphenpenalty=10000\tolerance=5000
                       \parindent=0pt\raggedright
                       \rm #1\hfill}}\bigskip \par\penalty 10000 %
}

\def\chfopen #1#2{\chapoddpage {\chapfonts
\vbox to 3in{\vfil \hbox to\hsize{\hfil #2} \hbox to\hsize{\hfil #1} \vfil}}%
\par\penalty 5000 %
}

\def\CHAPFopen{
\global\let\chapmacro=\chfopen
\global\let\unnumbchapmacro=\unnchfopen}

% Parameter controlling skip before section headings.

\newskip \subsecheadingskip  \subsecheadingskip = 17pt plus 8pt minus 4pt
\def\subsecheadingbreak{\dobreak \subsecheadingskip {-500}}

\newskip \secheadingskip  \secheadingskip = 21pt plus 8pt minus 4pt
\def\secheadingbreak{\dobreak \secheadingskip {-1000}}

% @paragraphindent  is defined for the Info formatting commands only.
\let\paragraphindent=\comment

% Section fonts are the base font at magstep2, which produces
% a size a bit more than 14 points in the default situation.

\def\secheading #1#2#3{\secheadingi {#2.#3\enspace #1}}
\def\plainsecheading #1{\secheadingi {#1}}
\def\secheadingi #1{{\advance \secheadingskip by \parskip %
\secheadingbreak}%
{\secfonts \vbox{\hyphenpenalty=10000\tolerance=5000
                 \parindent=0pt\raggedright
                 \rm #1\hfill}}%
\ifdim \parskip<10pt \kern 10pt\kern -\parskip\fi \penalty 10000 }


% Subsection fonts are the base font at magstep1,
% which produces a size of 12 points.

\def\subsecheading #1#2#3#4{\subsecheadingi {#2.#3.#4\enspace #1}}
\def\subsecheadingi #1{{\advance \subsecheadingskip by \parskip %
\subsecheadingbreak}%
{\subsecfonts \vbox{\hyphenpenalty=10000\tolerance=5000
                     \parindent=0pt\raggedright
                     \rm #1\hfill}}%
\ifdim \parskip<10pt \kern 10pt\kern -\parskip\fi \penalty 10000 }

\def\subsubsecfonts{\subsecfonts} % Maybe this should change:
				  % Perhaps make sssec fonts scaled
				  % magstep half
\def\subsubsecheading #1#2#3#4#5{\subsubsecheadingi {#2.#3.#4.#5\enspace #1}}
\def\subsubsecheadingi #1{{\advance \subsecheadingskip by \parskip %
\subsecheadingbreak}%
{\subsubsecfonts \vbox{\hyphenpenalty=10000\tolerance=5000
                       \parindent=0pt\raggedright
                       \rm #1\hfill}}%
\ifdim \parskip<10pt \kern 10pt\kern -\parskip\fi \penalty 10000}


\message{toc printing,}

% Finish up the main text and prepare to read what we've written
% to \contentsfile.

\newskip\contentsrightmargin \contentsrightmargin=1in
\def\startcontents#1{%
   \pagealignmacro
   \immediate\closeout \contentsfile
   \ifnum \pageno>0
      \pageno = -1		% Request roman numbered pages.
   \fi
   % Don't need to put `Contents' or `Short Contents' in the headline.
   % It is abundantly clear what they are.
   \unnumbchapmacro{#1}\def\thischapter{}%
   \begingroup   		% Set up to handle contents files properly.
      \catcode`\\=0  \catcode`\{=1  \catcode`\}=2  \catcode`\@=11
      \catcode`\^=7 % to see ^^e4 as \"a etc. juha@piuha.ydi.vtt.fi
      \raggedbottom             % Worry more about breakpoints than the bottom.
      \advance\hsize by -\contentsrightmargin % Don't use the full line length.
}


% Normal (long) toc.
\outer\def\contents{%
   \startcontents{\putwordTableofContents}%
      \input \jobname.toc
   \endgroup
   \vfill \eject
}

% And just the chapters.
\outer\def\summarycontents{%
   \startcontents{\putwordShortContents}%
      %
      \let\chapentry = \shortchapentry
      \let\unnumbchapentry = \shortunnumberedentry
      % We want a true roman here for the page numbers.
      \secfonts
      \let\rm=\shortcontrm \let\bf=\shortcontbf \let\sl=\shortcontsl
      \rm
      \advance\baselineskip by 1pt % Open it up a little.
      \def\secentry ##1##2##3##4{}
      \def\unnumbsecentry ##1##2{}
      \def\subsecentry ##1##2##3##4##5{}
      \def\unnumbsubsecentry ##1##2{}
      \def\subsubsecentry ##1##2##3##4##5##6{}
      \def\unnumbsubsubsecentry ##1##2{}
      \input \jobname.toc
   \endgroup
   \vfill \eject
}
\let\shortcontents = \summarycontents

% These macros generate individual entries in the table of contents.
% The first argument is the chapter or section name.
% The last argument is the page number.
% The arguments in between are the chapter number, section number, ...

% Chapter-level things, for both the long and short contents.
\def\chapentry#1#2#3{\dochapentry{#2\labelspace#1}{#3}}

% See comments in \dochapentry re vbox and related settings
\def\shortchapentry#1#2#3{%
  \tocentry{\shortchaplabel{#2}\labelspace #1}{\doshortpageno{#3}}%
}

% Typeset the label for a chapter or appendix for the short contents.
% The arg is, e.g. `Appendix A' for an appendix, or `3' for a chapter.
% We could simplify the code here by writing out an \appendixentry
% command in the toc file for appendices, instead of using \chapentry
% for both, but it doesn't seem worth it.
\setbox0 = \hbox{\shortcontrm \putwordAppendix }
\newdimen\shortappendixwidth \shortappendixwidth = \wd0

\def\shortchaplabel#1{%
  % We typeset #1 in a box of constant width, regardless of the text of
  % #1, so the chapter titles will come out aligned.
  \setbox0 = \hbox{#1}%
  \dimen0 = \ifdim\wd0 > \shortappendixwidth \shortappendixwidth \else 0pt \fi
  %
  % This space should be plenty, since a single number is .5em, and the
  % widest letter (M) is 1em, at least in the Computer Modern fonts.
  % (This space doesn't include the extra space that gets added after
  % the label; that gets put in in \shortchapentry above.)
  \advance\dimen0 by 1.1em
  \hbox to \dimen0{#1\hfil}%
}

\def\unnumbchapentry#1#2{\dochapentry{#1}{#2}}
\def\shortunnumberedentry#1#2{\tocentry{#1}{\doshortpageno{#2}}}

% Sections.
\def\secentry#1#2#3#4{\dosecentry{#2.#3\labelspace#1}{#4}}
\def\unnumbsecentry#1#2{\dosecentry{#1}{#2}}

% Subsections.
\def\subsecentry#1#2#3#4#5{\dosubsecentry{#2.#3.#4\labelspace#1}{#5}}
\def\unnumbsubsecentry#1#2{\dosubsecentry{#1}{#2}}

% And subsubsections.
\def\subsubsecentry#1#2#3#4#5#6{%
  \dosubsubsecentry{#2.#3.#4.#5\labelspace#1}{#6}}
\def\unnumbsubsubsecentry#1#2{\dosubsubsecentry{#1}{#2}}


% This parameter controls the indentation of the various levels.
\newdimen\tocindent \tocindent = 3pc

% Now for the actual typesetting. In all these, #1 is the text and #2 is the
% page number.
%
% If the toc has to be broken over pages, we would want to be at chapters
% if at all possible; hence the \penalty.
\def\dochapentry#1#2{%
   \penalty-3