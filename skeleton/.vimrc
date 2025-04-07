" Remove this line if you do not wish it to be overwritten by an update.

" When started as "evim", evim.vim will already have done these settings.
if v:progname =~? "evim"
  finish
endif

" Use Vim settings, rather then Vi settings (much better!).
" This must be first, because it changes other options as a side effect.
set nocompatible

" allow backspacing over everything in insert mode
set backspace=indent,eol,start

if has("vms")
  set nobackup		" do not keep a backup file, use versions instead
else
  set backup		" keep a backup file
endif
set history=50		" keep 50 lines of command line history
set ruler		" show the cursor position all the time
set showcmd		" display incomplete commands
set incsearch		" do incremental searching

" For Win32 GUI: remove 't' flag from 'guioptions': no tearoff menu entries
" let &guioptions = substitute(&guioptions, "t", "", "g")

" Don't use Ex mode, use Q for formatting
map Q gq

" This is an alternative that also works in block mode, but the deleted
" text is lost and it only works for putting the current register.
"vnoremap p "_dp

" Switch syntax highlighting on, when the terminal has colors
" Also switch on highlighting the last used search pattern.
if &t_Co > 2 || has("gui_running")
  syntax on
  set hlsearch
endif

" Only do this part when compiled with support for autocommands.
if has("autocmd")

  " Enable file type detection.
  " Use the default filetype settings, so that mail gets 'tw' set to 72,
  " 'cindent' is on in C files, etc.
  " Also load indent files, to automatically do language-dependent indenting.
  filetype plugin indent on

  " Put these in an autocmd group, so that we can delete them easily.
  augroup vimrcEx
  au!

  " For all text files set 'textwidth' to 78 characters.
  autocmd FileType text setlocal textwidth=78

  " When editing a file, always jump to the last known cursor position.
  " Don't do it when the position is invalid or when inside an event handler
  " (happens when dropping a file on gvim).
  autocmd BufReadPost *
    \ if line("'\"") > 0 && line("'\"") <= line("$") |
    \   exe "normal g`\"" |
    \ endif

  augroup END

  " PyLint - von Johannes
  "  autocmd FileType python compiler pylint
else

  set autoindent		" always set autoindenting on

endif " has("autocmd")

set nobackup
set nowrap
set tabstop=4
set encoding=utf-8
set noexpandtab tabstop=4 shiftwidth=4

set diffopt=filler,iwhite,icase,context:10
set linebreak

nnoremap <silent> c :call ToggleWrap()<CR>

function ToggleWrap()
  if &wrap
    call DisableDisplayWrapping()
  else
    call EnableDisplayWrapping()
  endif
endfunction

function ChooseWrap()
  let l:choice=confirm("Toggle Wrapping?", "&yes\n&no", 0)
  if l:choice==1
	call ToggleWrap()
  endif
endfunction

function EnableDisplayWrapping()
  if !&wrap
    setlocal wrap
    " don't skip wrapped lines
    nnoremap <buffer> <Up> gk
    nnoremap <buffer> <Down> gj
    nnoremap <buffer> <Home> g^
    nnoremap <buffer> <End> g$
    inoremap <buffer> <Up> <C-O>gk
    inoremap <buffer> <Down> <C-O>gj
    inoremap <buffer> <Home> <C-O>g^
    inoremap <buffer> <End> <C-O>g$
    vnoremap <buffer> <Up> gk
    vnoremap <buffer> <Down> gj
    vnoremap <buffer> <Home> g^
    vnoremap <buffer> <End> g$
  endif
endfunction
function DisableDisplayWrapping()
  if &wrap
    setlocal nowrap
    nunmap <buffer> <Up>
    nunmap <buffer> <Down>
    nunmap <buffer> <Home>
    nunmap <buffer> <End>
    iunmap <buffer> <Up>
    iunmap <buffer> <Down>
    iunmap <buffer> <Home>
    iunmap <buffer> <End>
    vunmap <buffer> <Up>
    vunmap <buffer> <Down>
    vunmap <buffer> <Home>
    vunmap <buffer> <End>
  endif
endfunction

function SOS()
	set guifont=Courier\ New\ 17
endfunction

" Ctrl-X goes to active keyword
" Ctrl-Y goes back
inoremap <C-X> <Esc>g<C-]>
noremap <C-X> g<C-]>
inoremap <C-S> <Esc>g<C-]>2<Enter>
noremap <C-S> g<C-]>2<Enter>
inoremap <C-Y> <Esc><C-T>
noremap <C-Y> <C-T>
set tags=tags,../tags
set autowrite

" Reset last position
set viminfo='10,\"100,:20,%,n~/.viminfo
au BufReadPost * if line("'\"") > 0|if line("'\"") <= line("$")|exe("norm '\"")|else|exe "norm $"|endif|endif

set foldmethod=marker

set guifont=Latin\ Modern\ Mono\ 12

" Previous and next on Ctrl-PageUp and Ctrl-PageDown (e.g., when using linter)
imap <C-PageUp> <Esc>:cp<Enter>zz:cc<Enter>
map <C-PageUp> :cp<Enter>zz:cc<Enter>
imap <C-PageDown> <Esc>:cn<Enter>zz:cc<Enter>
map <C-PageDown> :cn<Enter>zz:cc<Enter>

" Update copyright year when writing Python code
autocmd FileType python autocmd BufWritePre <buffer> let pos = winsaveview() | :%s/^#\s\+Copyright (C) \d\+-\zs\d\+\ze Johannes Bauer$/\=strftime("%Y")/e | call winrestview(pos) |unlet! pos

" Remove tailing spaces for a number of file types
autocmd FileType python autocmd BufWritePre <buffer> :%s/\s\+$//e
autocmd FileType javascript autocmd BufWritePre <buffer> :%s/\s\+$//e
autocmd FileType c autocmd BufWritePre <buffer> :%s/\s\+$//e
autocmd FileType cpp autocmd BufWritePre <buffer> :%s/\s\+$//e
autocmd FileType html autocmd BufWritePre <buffer> :%s/\s\+$//e
autocmd FileType css autocmd BufWritePre <buffer> :%s/\s\+$//e
autocmd FileType json autocmd BufWritePre <buffer> :%s/\s\+$//e
autocmd FileType xml autocmd BufWritePre <buffer> :%s/\s\+$//e
autocmd FileType rust autocmd BufWritePre <buffer> :%s/\s\+$//e

" Allow pasting with middle mouse button
set clipboard=unnamedplus
