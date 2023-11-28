bsseclevel % used to calculate proper heading level
\newcount\secbase\secbase=0 % @raise/lowersections modify this count

% @raisesections: treat @section as chapter, @subsection as section, etc.
\def\raisesections{\global\advance\secbase by -1}
\let\up=\raisesections % original BFox name

% @lowersections: treat @chapter as section, @section as subsection, etc.
\def\lowersections{\global\advance\secbase by 1}
\let\down=\lowersections % original BFox name

% Choose a numbered-heading macro
% #1 is heading level if unmodified by @raisesections or @lowersections
% #2 is text for heading
\def\numhead#1#2{\absseclevel=\secbase\advance\absseclevel by #1
\ifcase\absseclevel
  \chapterzzz{#2}
\or
  \seczzz{#2}
\or
  \numberedsubseczzz{#2}
\or
  \numberedsubsubseczzz{#2}
\else
  \ifnum \absseclevel<0
    \chapterzzz{#2}
  \else
    \numberedsubsubseczzz{#2}
  \fi
\fi
}

% like \numhead, but chooses appendix heading levels
\def\apphead#1#2{\absseclevel=\secbase\advance\absseclevel by #1
\ifcase\absseclevel
  \appendixzzz{#2}
\or
  \appendixsectionzzz{#2}
\or
  \appendixsubseczzz{#2}
\or
  \appendixsubsubseczzz{#2}
\else
  \ifnum \absseclevel<0
    \appendixzzz{#2}
  \else
    \appendixsubsubseczzz{#2}
  \fi
\fi
}

% like \numhead, but chooses numberless heading levels
\def\unnmhead#1#2{\absseclevel=\secbase\advance\absseclevel by #1
\ifcase\absseclevel
  \unnumberedzzz{#2}
\or
  \unnumberedseczzz{#2}
\or
  \unnumberedsubseczzz{#2}
\or
  \unnumberedsubsubseczzz{#2}
\else
  \ifnum \absseclevel<0
    \unnumberedzzz{#2}
  \else
    \unnumberedsubsubseczzz{#2}
  \fi
\fi
}


\def\thischaptername{No Chapter Title}
\outer\def\chapter{\parsearg\chapteryyy}
\def\chapteryyy #1{\numhead0{#1}} % normally numhead0 calls chapterzzz
\def\chapterzzz #1{\seccheck{chapter}%
\secno=0 \subsecno=0 \subsubsecno=0
\global\advance \chapno by 1 \message{Chapter \the\chapno}%
\chapmacro {#1}{\the\chapno}%
\gdef\thissection{#1}%
\gdef\thischaptername{#1}%
% We don't substitute the actual chapter name into \thischapter
% because we don't want its macros evaluated now.
\xdef\thischapter{\putwordChapter{} \the\chapno: \noexpand\thischaptername}%
{\chapternofonts%
\edef\temp{{\realbackslash chapentry {#1}{\the\chapno}{\noexpand\folio}}}%
\escapechar=`\\%
\write \contentsfile \temp  %
\donoderef %
\global\let\section = \numberedsec
\global\let\subsection = \numberedsubsec
\global\let\subsubsection = \numberedsubsubsec
}}

\outer\def\appendix{\parsearg\appendixyyy}
\def\appendixyyy #1{\apphead0{#1}} % normally apphead0 calls appendixzzz
\def\appendixzzz #1{\seccheck{appendix}%
\secno=0 \subsecno=0 \subsubsecno=0
\global\advance \appendixno by 1 \message{Appendix \appendixletter}%
\chapmacro {#1}{\putwordAppendix{} \appendixletter}%
\gdef\thissection{#1}%
\gdef\thischaptername{#1}%
\xdef\thischapter{\putwordAppendix{} \appendixletter: \noexpand\thischaptername}%
{\chapternofonts%
\edef\temp{{\re