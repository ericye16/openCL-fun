a = csvread("output.csv");
for z = 1:300
b = 512 * z - 511;
e = 512 * z;
scatter3(a(b:e, 1), a(b:e, 2), a(b:e, 3));
axis([0. 10, 0, 10, 0, 10]);
sleep(1);
endfor
