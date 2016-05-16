filetype on
filetype plugin on
filetype indent on

syntax enable
syntax on

set autoread
set background=dark
set cursorcolumn
set completeopt=preview,menu
set noexpandtab
set foldcolumn=0
set hlsearch
set incsearch
set ignorecase
set laststatus=2
set nocompatible
set number
set ruler
set report=0
set shiftwidth=4
set softtabstop=4
set statusline=%F%m%r%w\ [POS=%l,%v][%p%%]
set showcmd
set tabstop=4
set wildmenu
colorscheme delek

inoremap ( ()<LEFT>
inoremap [ []<LEFT>
inoremap { {	}<UP><RIGHT>
inoremap " ""<LEFT>
inoremap ' ''<LEFT>
