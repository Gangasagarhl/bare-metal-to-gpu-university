// The full adder of F1-13 (two half adders and an OR gate), written as gates.
module full_adder(input wire a, input wire b, input wire cin,
                  output wire s, output wire cout);
    wire s1, c1, c2;
    xor g1(s1, a, b);
    and g2(c1, a, b);
    xor g3(s, s1, cin);
    and g4(c2, s1, cin);
    or  g5(cout, c1, c2);
endmodule
