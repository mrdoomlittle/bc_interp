../bcc/bin/bcc -I ../bcc/lib -i test.bc -o test.rbc && sh compile.sh && ./bin/bci -s -slice -exec test.rbc
