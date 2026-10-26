"""
generate_diagrams.py
--------------------
Generates all six project diagrams as PNG files.

Usage:
    pip install matplotlib numpy
    python scripts/generate_diagrams.py

Output: docs/figures/*.png
"""

import os, sys
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
from matplotlib.patches import FancyBboxPatch, FancyArrowPatch
import numpy as np

OUTDIR = os.path.join(os.path.dirname(__file__), "..", "docs", "figures")
os.makedirs(OUTDIR, exist_ok=True)

NAVY  = "#001F5B"
GOLD  = "#C8993A"
LBLUE = "#BFD4FF"
WHITE = "#FFFFFF"
TEAL  = "#1A7A8A"

# ─────────────────────────────────────────────────────────────────────────────
# 1. IoT 4-layer architecture
# ─────────────────────────────────────────────────────────────────────────────
def fig_arch():
    fig, ax = plt.subplots(figsize=(9, 5.5))
    ax.set_xlim(0, 9); ax.set_ylim(0, 5.5); ax.axis("off")
    fig.patch.set_facecolor(NAVY)

    layers = [
        ("Application Layer",  "Web Dashboard / MQTT App",            4.8, GOLD),
        ("Processing Layer",   "ESP8266 NodeMCU\nMQTT/TLS · HTTP",    3.6, TEAL),
        ("Network Layer",      "Wi-Fi 802.11 b/g/n\nHiveMQ Cloud",    2.4, "#0052A5"),
        ("Perception Layer",   "Arduino UNO\nDHT11 · LDR · Relays",   1.2, "#003087"),
    ]

    for label, sub, y, col in layers:
        rect = FancyBboxPatch((0.4, y - 0.45), 8.2, 0.9,
                              boxstyle="round,pad=0.05", linewidth=0,
                              facecolor=col, zorder=2)
        ax.add_patch(rect)
        ax.text(1.0, y, label, va="center", ha="left",
                color=GOLD, fontsize=12, fontweight="bold")
        ax.text(7.5, y, sub, va="center", ha="right",
                color=LBLUE, fontsize=9)

    # arrows
    for y in [1.65, 2.85, 4.05]:
        ax.annotate("", xy=(4.5, y + 0.5), xytext=(4.5, y),
                    arrowprops=dict(arrowstyle="->", color=GOLD, lw=2))

    ax.set_title("IoT 4-Layer Architecture", color=WHITE, fontsize=14,
                 fontweight="bold", pad=8)
    fig.tight_layout()
    path = os.path.join(OUTDIR, "fig_arch.png")
    fig.savefig(path, dpi=150, facecolor=fig.get_facecolor())
    plt.close(fig)
    print(f"  saved {path}")

# ─────────────────────────────────────────────────────────────────────────────
# 2. Arduino firmware flowchart
# ─────────────────────────────────────────────────────────────────────────────
def fig_flow():
    fig, ax = plt.subplots(figsize=(6, 9))
    ax.set_xlim(0, 6); ax.set_ylim(0, 9); ax.axis("off")
    fig.patch.set_facecolor(WHITE)

    def box(x, y, w, h, text, col=NAVY, tcol=WHITE, shape="rect"):
        if shape == "diamond":
            dx, dy = w/2, h/2
            pts = [[x, y+dy],[x+dx, y],[x+2*dx, y+dy],[x+dx, y+2*dy]]
            from matplotlib.patches import Polygon
            p = Polygon(pts, closed=True, facecolor=col, edgecolor=GOLD, lw=1.5, zorder=2)
            ax.add_patch(p)
            ax.text(x + dx, y + dy, text, ha="center", va="center",
                    color=tcol, fontsize=8.5, fontweight="bold", zorder=3)
        else:
            r = FancyBboxPatch((x, y), w, h, boxstyle="round,pad=0.05",
                               facecolor=col, edgecolor=GOLD, lw=1.5, zorder=2)
            ax.add_patch(r)
            ax.text(x + w/2, y + h/2, text, ha="center", va="center",
                    color=tcol, fontsize=8.5, fontweight="bold", zorder=3)

    def arr(x1, y1, x2, y2):
        ax.annotate("", xy=(x2, y2), xytext=(x1, y1),
                    arrowprops=dict(arrowstyle="->", color=NAVY, lw=1.5))

    box(1.5, 8.3, 3, 0.5, "START / setup()", GOLD, NAVY)
    arr(3.0, 8.3, 3.0, 7.7)
    box(1.5, 7.1, 3, 0.5, "Read DHT11 + LDR")
    arr(3.0, 7.1, 3.0, 6.5)
    box(0.8, 5.7, 2.4, 0.6, "autoMode?", "#003087", WHITE, "diamond")
    arr(3.0, 6.5, 3.0, 6.0)
    # yes branch
    ax.text(1.8, 5.45, "YES", fontsize=8, color=TEAL, ha="center")
    arr(1.8, 5.7, 1.8, 5.1)
    box(0.5, 4.6, 2.6, 0.4, "LDR < 200?", "#0052A5", WHITE, "diamond")
    arr(1.8, 4.6, 1.8, 4.1)
    box(0.3, 3.6, 2.4, 0.4, "All relays ON")
    # no branch (manual)
    ax.text(3.25, 5.85, "NO", fontsize=8, color=TEAL)
    box(3.0, 5.6, 2.5, 0.4, "Serial CMD?")
    arr(4.25, 5.6, 4.25, 5.1)
    box(3.0, 4.6, 2.5, 0.4, "Set relay[i]")
    # merge
    arr(3.0, 7.1, 3.0, 6.5)
    arr(1.8, 3.6, 1.8, 3.1)
    arr(4.25, 4.6, 4.25, 3.1)
    box(1.5, 2.6, 3.0, 0.4, "Send SENSOR line to ESP")
    arr(3.0, 2.6, 3.0, 2.1)
    box(1.5, 1.6, 3.0, 0.4, "Update OLED")
    arr(3.0, 1.6, 3.0, 1.1)
    box(1.5, 0.6, 3.0, 0.4, "Delay 2 s → loop()", GOLD, NAVY)

    ax.set_title("Arduino Firmware Flowchart", color=NAVY,
                 fontsize=13, fontweight="bold", pad=8)
    fig.tight_layout()
    path = os.path.join(OUTDIR, "fig_flow.png")
    fig.savefig(path, dpi=150, facecolor=fig.get_facecolor())
    plt.close(fig)
    print(f"  saved {path}")

# ─────────────────────────────────────────────────────────────────────────────
# 3. User command flow
# ─────────────────────────────────────────────────────────────────────────────
def fig_userflow():
    fig, ax = plt.subplots(figsize=(10, 4.5))
    ax.set_xlim(0, 10); ax.set_ylim(0, 4.5); ax.axis("off")
    fig.patch.set_facecolor(NAVY)

    nodes = [
        (0.5, 2.0, 1.5, 0.7, "User\n(Browser / App)",  GOLD,    NAVY),
        (2.5, 2.0, 1.8, 0.7, "HiveMQ\nCloud MQTT",     "#0052A5", WHITE),
        (5.0, 2.0, 1.8, 0.7, "ESP8266\nNodeMCU",        TEAL,    WHITE),
        (7.5, 2.0, 1.8, 0.7, "Arduino\nUNO",            "#003087", WHITE),
    ]
    for x, y, w, h, label, fc, tc in nodes:
        r = FancyBboxPatch((x, y), w, h, boxstyle="round,pad=0.08",
                           facecolor=fc, edgecolor=GOLD, lw=1.5, zorder=2)
        ax.add_patch(r)
        ax.text(x + w/2, y + h/2, label, ha="center", va="center",
                color=tc, fontsize=9.5, fontweight="bold", zorder=3)

    arrows = [
        (2.0, 2.35, 2.5, 2.35, 'MQTT CMD\n{"room":"kitchen","state":"ON"}'),
        (4.3, 2.35, 5.0, 2.35, "MQTT subscribe"),
        (6.8, 2.35, 7.5, 2.35, "SoftSerial\nCMD,KITCHEN,ON"),
        (7.5, 2.0,  6.8, 2.0,  "SoftSerial\nSTATE,kitchen,ON"),
        (5.0, 2.0,  4.3, 2.0,  "MQTT publish\nhome/status"),
    ]
    for x1, y1, x2, y2, label in arrows:
        ax.annotate("", xy=(x2, y2), xytext=(x1, y1),
                    arrowprops=dict(arrowstyle="->", color=GOLD, lw=1.8))
        mx, my = (x1 + x2)/2, (y1 + y2)/2
        ax.text(mx, my + 0.25, label, ha="center", va="bottom",
                color=LBLUE, fontsize=7.5)

    ax.set_title("User Command Flow Diagram", color=WHITE,
                 fontsize=13, fontweight="bold", pad=6)
    fig.tight_layout()
    path = os.path.join(OUTDIR, "fig_userflow.png")
    fig.savefig(path, dpi=150, facecolor=fig.get_facecolor())
    plt.close(fig)
    print(f"  saved {path}")

# ─────────────────────────────────────────────────────────────────────────────
# 4. System deployment view
# ─────────────────────────────────────────────────────────────────────────────
def fig_deploy():
    fig, ax = plt.subplots(figsize=(10, 5.5))
    ax.set_xlim(0, 10); ax.set_ylim(0, 5.5); ax.axis("off")
    fig.patch.set_facecolor(WHITE)

    components = [
        (0.3, 3.8, 2.2, 1.2, "Arduino UNO\n+ Sensors\n+ Relays",    NAVY,    WHITE),
        (3.0, 3.8, 2.2, 1.2, "ESP8266\nNodeMCU\n(Wi-Fi GW)",        TEAL,    WHITE),
        (5.8, 3.8, 2.2, 1.2, "HiveMQ Cloud\nMQTT Broker\n(TLS 8883)","#0052A5",WHITE),
        (8.0, 3.8, 1.7, 1.2, "Mobile /\nDesktop\nBrowser",           GOLD,    NAVY),
        (3.0, 1.5, 2.2, 1.2, "Local HTTP\nServer :80\n(LAN only)",   "#C8D0E8", NAVY),
        (0.3, 1.5, 2.2, 1.2, "LAN Browser\n(direct IP)",             GOLD,    NAVY),
    ]
    for x, y, w, h, label, fc, tc in components:
        r = FancyBboxPatch((x, y), w, h, boxstyle="round,pad=0.08",
                           facecolor=fc, edgecolor=NAVY, lw=1.5, zorder=2)
        ax.add_patch(r)
        ax.text(x+w/2, y+h/2, label, ha="center", va="center",
                color=tc, fontsize=8.5, fontweight="bold", zorder=3)

    def arr(x1,y1,x2,y2,lbl="",col=NAVY):
        ax.annotate("", xy=(x2,y2), xytext=(x1,y1),
                    arrowprops=dict(arrowstyle="<->", color=col, lw=1.8))
        if lbl:
            ax.text((x1+x2)/2, (y1+y2)/2+0.12, lbl,
                    ha="center", fontsize=7.5, color=col)

    arr(2.5, 4.4, 3.0, 4.4, "SoftSerial\n1200 baud")
    arr(5.2, 4.4, 5.8, 4.4, "MQTT/TLS")
    arr(8.0, 4.4, 8.0, 4.4)
    ax.annotate("", xy=(8.2, 4.4), xytext=(8.0, 4.4),
                arrowprops=dict(arrowstyle="->", color=NAVY, lw=1.8))
    ax.text(8.85, 4.5, "MQTT\nsubscribe", ha="center", fontsize=7.5, color=NAVY)
    arr(3.0, 3.8, 3.0, 2.7, "HTTP :80")
    arr(2.5, 2.1, 3.0, 2.1, "Browser")

    ax.set_title("System Deployment View", color=NAVY,
                 fontsize=13, fontweight="bold", pad=8)
    fig.tight_layout()
    path = os.path.join(OUTDIR, "fig_deploy.png")
    fig.savefig(path, dpi=150, facecolor=fig.get_facecolor())
    plt.close(fig)
    print(f"  saved {path}")

# ─────────────────────────────────────────────────────────────────────────────
# 5. AUTO vs MANUAL operational view
# ─────────────────────────────────────────────────────────────────────────────
def fig_opview():
    fig, axes = plt.subplots(1, 2, figsize=(10, 5))
    fig.patch.set_facecolor(NAVY)

    for ax in axes:
        ax.set_facecolor(NAVY)
        for spine in ax.spines.values(): spine.set_visible(False)

    t = np.linspace(0, 24, 500)

    # AUTO: LDR-based relay trace
    ax = axes[0]
    ldr = np.where((t > 6) & (t < 18), 0.8, 0.15)
    relay = np.where(ldr < 0.3, 1, 0)
    ax.fill_between(t, ldr, alpha=0.4, color=GOLD)
    ax.plot(t, ldr, color=GOLD, lw=2, label="LDR (normalised)")
    ax.step(t, relay, color=LBLUE, lw=2, where="post", label="Relay state")
    ax.axhline(0.3, color="red", ls="--", lw=1.5, label="Threshold 200")
    ax.set_xlabel("Hour of day", color=WHITE, fontsize=10)
    ax.set_title("AUTO Mode — 24 h cycle", color=WHITE, fontsize=11, fontweight="bold")
    ax.tick_params(colors=WHITE)
    ax.legend(facecolor=NAVY, labelcolor=WHITE, fontsize=8)
    ax.set_xlim(0, 24); ax.set_ylim(-0.1, 1.2)

    # MANUAL: random commands over time
    ax2 = axes[1]
    cmds = [(3.0, "KITCHEN ON"), (7.5, "HALL ON"), (11.0, "KITCHEN OFF"),
            (14.5, "ALL ON"), (19.0, "BEDROOM OFF"), (22.0, "ALL OFF")]
    state_y = {"KITCHEN": 0.2, "HALL": 0.45, "BEDROOM": 0.7, "ALL": 0.95}
    for xc, label in cmds:
        sy = state_y.get(label.split()[0], 0.5)
        ax2.annotate("", xy=(xc, sy), xytext=(xc, 0),
                     arrowprops=dict(arrowstyle="->", color=GOLD, lw=1.5))
        ax2.text(xc + 0.3, sy, label, color=LBLUE, fontsize=7.5)
    ax2.set_xlabel("Hour of day", color=WHITE, fontsize=10)
    ax2.set_title("MANUAL Mode — command events", color=WHITE,
                  fontsize=11, fontweight="bold")
    ax2.tick_params(colors=WHITE)
    ax2.set_xlim(0, 24); ax2.set_ylim(0, 1.2)

    fig.suptitle("AUTO vs. MANUAL Operational View", color=WHITE,
                 fontsize=13, fontweight="bold")
    fig.tight_layout()
    path = os.path.join(OUTDIR, "fig_opview.png")
    fig.savefig(path, dpi=150, facecolor=fig.get_facecolor())
    plt.close(fig)
    print(f"  saved {path}")

# ─────────────────────────────────────────────────────────────────────────────
# 6. UART CSV serial protocol sequence diagram
# ─────────────────────────────────────────────────────────────────────────────
def fig_serial():
    fig, ax = plt.subplots(figsize=(8, 6))
    ax.set_xlim(0, 8); ax.set_ylim(0, 6); ax.axis("off")
    fig.patch.set_facecolor(WHITE)

    # lifelines
    ax.plot([1.5, 1.5], [0.3, 5.5], color=NAVY, lw=2)
    ax.plot([6.5, 6.5], [0.3, 5.5], color=TEAL,  lw=2)
    ax.text(1.5, 5.7, "Arduino UNO", ha="center", fontsize=11,
            fontweight="bold", color=NAVY)
    ax.text(6.5, 5.7, "ESP8266\nNodeMCU", ha="center", fontsize=11,
            fontweight="bold", color=TEAL)

    seq = [
        (5.0, 1.5, 6.5, "SENSOR,30.0,62.0,95,DARK", NAVY, TEAL),
        (4.2, 1.5, 6.5, "SENSOR,30.1,62.0,80,DARK", NAVY, TEAL),
        (3.4, 6.5, 1.5, "CMD,KITCHEN,ON",            TEAL, NAVY),
        (2.6, 1.5, 6.5, "STATE,kitchen,ON",           NAVY, TEAL),
        (1.8, 1.5, 6.5, "SENSOR,30.2,61.8,95,DARK",  NAVY, TEAL),
        (1.0, 6.5, 1.5, "CMD,KITCHEN,OFF",            TEAL, NAVY),
    ]
    for y, x1, x2, label, fc, tc in seq:
        ax.annotate("", xy=(x2, y), xytext=(x1, y),
                    arrowprops=dict(arrowstyle="->",
                                   color=fc, lw=1.8))
        mx = (x1 + x2) / 2
        ax.text(mx, y + 0.15, label, ha="center", fontsize=8.5,
                color=fc, fontfamily="monospace")

    ax.set_title("UART CSV Serial Protocol Sequence", color=NAVY,
                 fontsize=13, fontweight="bold", pad=8)
    fig.tight_layout()
    path = os.path.join(OUTDIR, "fig_serial.png")
    fig.savefig(path, dpi=150, facecolor=fig.get_facecolor())
    plt.close(fig)
    print(f"  saved {path}")


if __name__ == "__main__":
    print("Generating diagrams →", OUTDIR)
    fig_arch()
    fig_flow()
    fig_userflow()
    fig_deploy()
    fig_opview()
    fig_serial()
    print("Done.")
