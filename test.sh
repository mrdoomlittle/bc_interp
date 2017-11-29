../bcc/bin/bcc -I ../bcc/lib -i test.bc -o test.rbc && sh compile.sh && ./bin/bci -s -slice -args 1 -exec test.rbc
