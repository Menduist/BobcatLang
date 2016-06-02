" Vim syntax file
" " Language: TIM
" " Maintainer: Tanguy CIZAIN
" " Latest Revision: 27 May 2016
"
if exists("b:current_syntax")
	finish
	endif

syn keyword basicLanguageKeywords func if while for else struct
" syn keyword basicTypes int
syn region	string		start=+"+ skip=+\\\\\|\\"+ end=+"+ extend
syn region block start="{" end="}" fold transparent
syn match	number		"\d\+"
syn match	number		"0x\x\+"

syn region  commentL	start="//" end="$" keepend contains=number
syn region  commentL	start="/\*" end="\*/" keepend contains=number
syn region  preprocessor	start="#" skip="\\$" end="$" keepend

syn match parenthesis "("
syn match function "\(\w*\)(" contains=parenthesis

let b:current_syntax = "tim"

hi def link string String
hi def link basicLanguageKeywords Statement
hi def link number Number
hi def link basicTypes Type
hi def link commentL Comment
hi def link preprocessor PreProc
hi def link function Identifier
