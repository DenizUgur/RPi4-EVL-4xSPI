#!/usr/local/bin/python3

import socket
import time
import threading
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

HOST = "127.0.0.1"
SIZE = 45  # for 8 decimal points, set 41 for 7 decimal points

ENCODERS = [[None] * 4, [None] * 4, [None] * 4, [None] * 4]
ENCODERS_ZERO = [[None] * 4, [None] * 4, [None] * 4, [None] * 4]
HISTORY = [[[0] * 100] * 4, [[0] * 100] * 4, [[0] * 100] * 4, [[0] * 100] * 4]


def listen_rpi(index):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        s.bind((HOST, 8080 + index))
        s.listen()
        print(f"Port {8080 + index} listening")

        conn, addr = s.accept()

        with conn:
            print("Connected by", addr)
            while True:
                data = conn.recv(SIZE)
                if not data:
                    break

                data = data.decode("utf-8")
                data = list(map(float, str(data[1:-1]).split(" ")))

                if all([enc is None for enc in ENCODERS[index]]):
                    ENCODERS_ZERO[index] = data

                for i, d in enumerate(data):
                    data[i] = d - ENCODERS_ZERO[index][i]

                ENCODERS[index] = data

                time.sleep(0.05)


def relay_encoders(i):
    for i, enc_set in enumerate(ENCODERS):
        for ii, enc in enumerate(enc_set):
            HISTORY[i][ii].pop(0)
            HISTORY[i][ii].append(enc)

    for y, col in enumerate(axs):
        for x, ax in enumerate(col):
            ax.cla()
            ax.plot(HISTORY[x][y])
            ax.set_title(f"SET: {x} ENC: {y}", size=10)


threads = [threading.Thread(target=listen_rpi, args=(i,)) for i in range(4)]

for t in threads:
    t.start()

fig, axs = plt.subplots(4, 4, figsize=(9, 9), sharex=True, sharey=True)
fig.tight_layout()
ani = FuncAnimation(fig, relay_encoders, interval=100)

plt.show()