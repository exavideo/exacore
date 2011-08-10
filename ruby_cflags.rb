require 'mkmf'

# FIXME this is a really really ugly hack job
puts "-I#{$arch_hdrdir} -I#{$hdrdir}/ruby/backward -I#{$hdrdir} -I#{$srcdir}".gsub(/\$\(arch\)/, CONFIG["arch"])

