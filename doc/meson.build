custom_target(
  'xfce4-terminal.1',
  input: 'xfce4-terminal.xml',
  output: 'xfce4-terminal.1',
  command: [xsltproc, '--nonet', '--output', '@OUTPUT@', 'http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl', '@INPUT@'],
  install: true,
  install_dir: get_option('prefix') / get_option('mandir') / 'man1',
)
