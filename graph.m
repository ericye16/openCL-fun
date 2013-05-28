a = csvread("output.csv");
for z = 1:300
b = 512 * z - 511;
e = 512 * z;
scatter3(a(b:e, 1), a(b:e, 3), a(b:e, 2), 30);
axis([-2. 10, -2, 10,-2, 10]);
usleep(70000);
endfor
