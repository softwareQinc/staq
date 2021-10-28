module top ( a, b, c, d, e );
  input a, b, c, d;
  output e;
  wire subprod1, subprod2;
  assign subprod1 = a & b;
  assign subprod2 = c & d;
  assign e = subprod1 & subprod2;
endmodule
