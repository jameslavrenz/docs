" Default vim config--no plugins added
set nocompatible " Use Vim without regard to Vi

filetype off " Do not detect file type

set number
set tabstop=4
set hlsearch
set t_Co=256 " Consider removing this

set background=light
colorscheme default

set wildmenu
set colorcolumn=80

syntax on
set laststatus=2

set splitbelow splitright

let mapleader = " "

nnoremap <silent> <C-h> :vertical resize +2<CR>
nnoremap <silent> <C-l> :vertical resize -2<CR>
nnoremap <silent> <C-k> :resize +2<CR>
nnoremap <silent> <C-j> :resize -2<CR>

nnoremap <Leader>e :NERDTreeToggle<CR>

