preset_files = [
  'black-on-white.desktop.in',
  'dark-pastels.desktop.in',
  'green-on-black.desktop.in',
  'solarized-dark.desktop.in',
  'solarized-light.desktop.in',
  'tango.desktop.in',
  'white-on-black.desktop.in',
  'xterm.desktop.in',
]

foreach file : preset_files
  i18n.merge_file(
    input: file,
    output: fs.replace_suffix(fs.stem(file), '.theme'),
    po_dir: '..' / 'po',
    type: 'desktop',
    install: true,
    install_dir: get_option('prefix') / get_option('datadir') / 'xfce4' / 'terminal' / 'colorschemes',
  )
endforeach
