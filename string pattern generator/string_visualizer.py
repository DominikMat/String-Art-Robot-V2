import turtle
import math
import random
import time
import os
from itertools import combinations

# --- CONFIGURATION ---
NUM_PINS = 126          # Number of nails on the ring
RADIUS = 300            # Radius of the ring in pixels
WINDOW_SIZE = 800       # Size of the window
ANIMATION_SPEED = 0     # 0 = fastest
RESET_DELAY = 100       # Short delay (in ms) after drawing

# Zmienne globalne do obsługi zapisu
CURRENT_SEQUENCE = []
CURRENT_NAME = ""
CURRENT_PARAMS = ""
CURRENT_IDX = 0
NEED_REDRAW = True

if not os.path.exists("generated"):
    os.makedirs("generated")

# Calculate position (x, y) for a given nail number
def get_pin_coords(pin_index):
    angle = (2 * math.pi * pin_index) / NUM_PINS
    x = RADIUS * math.cos(angle)
    y = RADIUS * math.sin(angle)
    return x, y

# ==========================================
# PART 1: SEQUENCE GENERATORS (All 12+)
# Each function returns a LIST of nail numbers (int)
# ==========================================

def pattern_multiplier(m=2):
    """Generuje kardioidy i inne epicykloidy. m=NUM_PINS/2 daje średnicę."""
    sequence = []
    for i in range(NUM_PINS):
        sequence.append(i)
        sequence.append(int(i * m) % NUM_PINS)
    return sequence

def pattern_starlike_fill(skip_factor=0.45):
    """Wypełnia środek poprzez skoki bliskie połowie obwodu (np. 0.5 * 100 = 50)."""
    skip = int(NUM_PINS * skip_factor)
    sequence = [0]
    current = 0
    for _ in range(NUM_PINS * 2):
        current = (current + skip) % NUM_PINS
        sequence.append(current)
        if current == 0: break
    return sequence

def pattern_web(step=10):
    """Tworzy siatkę łącząc każdy gwóźdź z gwoździem o 'step' dalej."""
    sequence = []
    for i in range(NUM_PINS):
        sequence.append(i)
        sequence.append((i + step) % NUM_PINS)
    return sequence

def pattern_nephroid():
    """Multiplier = 3, wypełnia środek bardziej złożonym kształtem."""
    return pattern_multiplier(3)

def pattern_star_skip(skip=13):
    """1. Star/Polygon: Skips a constant number of nails (skip)."""
    sequence = [0]
    current = 0
    for _ in range(NUM_PINS * 2): 
        current = (current + skip) % NUM_PINS
        if current == sequence[0]:
            sequence.append(current)
            break
        sequence.append(current)
    return sequence

def pattern_envelope_curve(offset=1):
    """2. Envelope Curve: Creates a hyperbolic curve (offset is the step left/right)."""
    sequence = []
    for i in range(NUM_PINS):
        pin_a = i
        pin_b = (NUM_PINS - 1 - i + offset) % NUM_PINS
        sequence.append(pin_a)
        sequence.append(pin_b)
    sequence.append(0)
    return sequence

def pattern_zigzag_layer(offset=12):
    """3. Layered Zig-Zag: Jumps forward by 'offset', then moves back a small step."""
    sequence = [0]
    current = 0
    for i in range(NUM_PINS * 2): 
        forward = (current + offset) % NUM_PINS
        sequence.append(forward)
        current = (current + 1) % NUM_PINS
        sequence.append(current)
        if current == 0 and i > NUM_PINS / 2:
            break
    return sequence

def pattern_flower_mandala():
    """4. Mandala/Flower: Connects a near point with an opposite point, creating a circular indentation."""
    sequence = []
    half = NUM_PINS // 2
    for i in range(NUM_PINS):
        sequence.append(i)          
        sequence.append((i + half) % NUM_PINS) 
    sequence.append(0) 
    return sequence

def pattern_chaos_random():
    """5. Chaos: Random sequence visiting all nails once."""
    pins = list(range(NUM_PINS))
    random.shuffle(pins)
    pins.append(pins[0])
    return pins

def pattern_double_bounce(jump_a=11, jump_b=10):
    """6. Double Bounce: Switches between two different 'skips'."""
    sequence = [0]
    current = 0
    for i in range(NUM_PINS * 2): 
        if i % 2 == 0:
            current = (current + jump_a) % NUM_PINS
        else:
            current = (current + jump_b) % NUM_PINS
        sequence.append(current)
        if current == sequence[0] and i > 1:
            break
    return sequence

def pattern_sierpinski_style(divisor=3):
    """7. Sierpinski Style: Uses modulo (e.g., 3) to choose the next point."""
    sequence = [0]
    current = 0
    for i in range(NUM_PINS * 2): 
        current = (current + divisor) % NUM_PINS 
        sequence.append(current)
        if current == sequence[0] and i > 1:
            break
    return sequence

def pattern_offset_cardioid(multiplier=2):
    """8. Cardioid (Classic): Connects pin 'i' to pin 'i * multiplier' (creates a heart/kidney shape)."""
    sequence = []
    for i in range(NUM_PINS):
        sequence.append(i)
        next_pin = (i * multiplier) % NUM_PINS
        sequence.append(next_pin)
    sequence.append(0)
    return sequence

def pattern_progressive_spiral(start_jump=1, step_increase=1):
    """9. Progressive Spiral: Increases 'skip' by a constant value each step."""
    sequence = [0]
    current = 0
    jump = start_jump
    for i in range(NUM_PINS * 4): # Longer loop for a better spiral
        current = (current + jump) % NUM_PINS
        sequence.append(current)
        jump += step_increase
        if current == sequence[0] and i > NUM_PINS:
            break
    return sequence

def pattern_inward_outward(in_step=1, out_step=15):
    """10. Inward/Outward: Alternates jumping forward by a small value and back by a large one."""
    sequence = [0]
    current = 0
    for i in range(NUM_PINS * 2):
        if i % 2 == 0:
            current = (current + in_step) % NUM_PINS
        else:
            current = (current + out_step) % NUM_PINS
        sequence.append(current)
        if current == sequence[0] and i > 1:
            break
    return sequence

def pattern_triple_star(skip_1=3, skip_2=8):
    """11. Triple Star: Uses three different jumps cyclically."""
    sequence = [0]
    current = 0
    skips = [skip_1, skip_2, skip_1 + skip_2]
    
    for i in range(NUM_PINS * 3):
        jump = skips[i % 3]
        current = (current + jump) % NUM_PINS
        sequence.append(current)
        if current == sequence[0] and i > 2:
            break
    return sequence

def pattern_half_and_quarter(skip_half=16, skip_quarter=8):
    """12. Half & Quarter: Connects points separated by half and quarter of the circumference."""
    sequence = [0]
    current = 0
    skips = [skip_half, skip_quarter]
    
    for i in range(NUM_PINS * 2):
        jump = skips[i % 2]
        current = (current + jump) % NUM_PINS
        sequence.append(current)
        if current == sequence[0] and i > 1:
            break
    return sequence
    
def pattern_full_coverage():
    """13. Full Coverage: Draws lines between all pairs of pins (very dense)."""
    sequence = []
    for i, j in combinations(range(NUM_PINS), 2):
        sequence.append(i)
        sequence.append(j)
    return sequence


# ==========================================
# PART 2: SEQUENCE CONFIGURATION AND AUTO-GENERATION
# ==========================================

# List of dictionaries for automatic sequence generation
SEQUENCE_LIST = [
    # Format: [Name, Function, List of parameter values]
    # ["1. Star (Skip)", pattern_star_skip, [7, 13, 3]],
    # ["2. Envelope Curve", pattern_envelope_curve, [1, 5]],
    # ["3. ZigZag Layer", pattern_zigzag_layer, [12, 5]],
    # ["4. Flower Mandala", pattern_flower_mandala, [None]],
    # ["5. Double Bounce", pattern_double_bounce, [(7, 15),(11,17)]],
    # ["6. Sierpinski Style", pattern_sierpinski_style, [3, 5]],
    # ["7. Offset Cardioid", pattern_offset_cardioid, [1,2,3,4,5,6,7,8,9,10]],
    # ["8. Progressive Spiral", pattern_progressive_spiral, [(1, 2), (3, 1)]],
    # ["9. Inward Outward", pattern_inward_outward, [(2, 10)]],
    # ["10. Triple Star", pattern_triple_star, [(3, 8), (4, 7)]],
    # ["11. Half & Quarter", pattern_half_and_quarter, [(14, 6)]],
    # ["12. Chaos Random", pattern_chaos_random, [None]],
    # ["13. Full Coverage", pattern_full_coverage, [None]]

    ["Cross", pattern_multiplier, [NUM_PINS // 2, NUM_PINS // 2 + 1]],
    ["Nephroid", pattern_multiplier, [3]],
    ["Cardioid", pattern_multiplier, [2]],
    ["Star", pattern_starlike_fill, [0.48, 0.37, 0.41]],
    ["Web", pattern_web, [20, 35, 45]],
    ["Abstract", pattern_multiplier, [1.5, 2.5]],
    ["Offset", pattern_offset_cardioid, [1,2,3,4,5,6,7,8,9,10]],
]

ALL_TASKS = []
for name, func, params in SEQUENCE_LIST:
    for p in params:
        ALL_TASKS.append({'name': name, 'func': func, 'param': p})

# ==========================================
# ZAPIS DO PLIKU GCODE
# ==========================================

def save_to_gcode():
    global CURRENT_SEQUENCE, CURRENT_NAME, CURRENT_PARAMS
    if not CURRENT_SEQUENCE: return

    # Krótka nazwa: pierwsze 4 litery nazwy + param (max 10 znaków razem)
    short_name = CURRENT_NAME[:4].lower().strip()
    param_str = str(CURRENT_PARAMS).replace(".", "")[:4]
    filename = f"generated/{short_name}{param_str}.gcode"
    
    try:
        with open(filename, "w") as f:
            f.write(f"NAIL_NUMBER={NUM_PINS}\n")
            f.write(f"SEQUENCE_LENGTH={len(CURRENT_SEQUENCE)}\n")
            f.write("SEQUENCE_START\n")
            for pin in CURRENT_SEQUENCE:
                f.write(f"{pin}\n")
            f.write("SEQUENCE_END\n")
        
        print(f"Zapisano: {filename}")
        show_message("SAVED!")
    except Exception as e:
        print(f"Błąd zapisu: {e}")

# ==========================================
# PART 3: VISUALIZATION (TURTLE)
# ==========================================

def go_next():
    global CURRENT_IDX, NEED_REDRAW
    CURRENT_IDX = (CURRENT_IDX + 1) % len(ALL_TASKS)
    NEED_REDRAW = True

def go_prev():
    global CURRENT_IDX, NEED_REDRAW
    CURRENT_IDX = (CURRENT_IDX - 1) % len(ALL_TASKS)
    NEED_REDRAW = True

def show_message(txt):
    m = turtle.Turtle()
    m.hideturtle(); m.penup(); m.color("yellow")
    m.goto(0, -RADIUS - 60)
    m.write(txt, align="center", font=("Arial", 12, "bold"))
    time.sleep(0.4); m.clear()

def setup_turtle():
    screen = turtle.Screen()
    screen.setup(WINDOW_SIZE, WINDOW_SIZE)
    screen.bgcolor("black")
    screen.title("String Art: Arrows (Prev/Next) | G (Save)")
    
    t_line = turtle.Turtle()
    t_line.hideturtle()
    
    t_text = turtle.Turtle()
    t_text.hideturtle(); t_text.penup(); t_text.goto(0, RADIUS + 20)
    
    screen.listen()
    screen.onkey(go_next, 'Right')
    screen.onkey(go_prev, 'Left')
    screen.onkey(save_to_gcode, 'g')
    screen.onkey(save_to_gcode, 'G')
    
    return t_line, t_text, screen

def main():
    global NEED_REDRAW, CURRENT_SEQUENCE, CURRENT_NAME, CURRENT_PARAMS
    t_line, t_text, screen = setup_turtle()
    
    while True:
        if NEED_REDRAW:
            screen.tracer(0)
            t_line.clear()
            t_text.clear()
            
            # Pobierz aktualne zadanie
            task = ALL_TASKS[CURRENT_IDX]
            CURRENT_NAME = task['name']
            CURRENT_PARAMS = task['param']
            CURRENT_SEQUENCE = task['func'](task['param'])
            
            # Rysuj piny
            t_line.penup()
            t_line.color("#330000")
            for i in range(NUM_PINS):
                x, y = get_pin_coords(i)
                t_line.goto(x, y); t_line.dot(3)
            
            # Rysuj wzór
            t_line.color("#00FFFF")
            t_line.pensize(1)
            if CURRENT_SEQUENCE:
                sx, sy = get_pin_coords(CURRENT_SEQUENCE[0])
                t_line.goto(sx, sy); t_line.pendown()
                for pin in CURRENT_SEQUENCE[1:]:
                    x, y = get_pin_coords(pin)
                    t_line.goto(x, y)
            
            # Tekst informacyjny
            t_text.color("white")
            t_text.goto(0, RADIUS + 40)
            t_text.write(f"{CURRENT_NAME} (p: {CURRENT_PARAMS})", align="center", font=("Arial", 14, "bold"))
            t_text.goto(0, RADIUS + 20)
            t_text.color("gray")
            t_text.write(f"Pattern {CURRENT_IDX+1}/{len(ALL_TASKS)} | Arrows: Move | G: Save", align="center", font=("Arial", 10, "normal"))
            
            screen.update()
            NEED_REDRAW = False
            
        screen.update()
        time.sleep(0.01)

if __name__ == "__main__":
    main()