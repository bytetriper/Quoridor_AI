import os
os.chdir("D:\\VS_C\\homework\\PPCA\\AI\\Minimax")
os.system("g++ -std=c++17 -o sample sample.cpp -Wall -Wextra")
os.system("python judge.py sample.exe sample_prun.exe")
