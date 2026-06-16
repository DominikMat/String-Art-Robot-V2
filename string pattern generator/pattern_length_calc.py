import math
import os
import tkinter as tk
from tkinter import filedialog

def calculate_string_length(file_path, ring_diameter_mm=366.0):
    if not os.path.exists(file_path):
        print(f"Błąd: Nie znaleziono pliku '{file_path}'")
        return

    nail_number = 0
    sequence = []
    in_sequence = False

    # 1. Wczytywanie pliku
    with open(file_path, 'r', encoding='utf-8') as file:
        for line in file:
            line = line.strip()
            if not line:
                continue
            
            if line.startswith("NAIL_NUMBER="):
                nail_number = int(line.split("=")[1])
            elif line == "SEQUENCE_START":
                in_sequence = True
            elif line == "SEQUENCE_END":
                in_sequence = False
            elif in_sequence:
                try:
                    sequence.append(int(line))
                except ValueError:
                    pass # Ignoruj linie, które nie są liczbami

    # Walidacja danych
    if nail_number == 0 or len(sequence) < 2:
        print("Błąd: Niepoprawny plik gcode (brak zmiennej NAIL_NUMBER lub pusta sekwencja).")
        return

    # 2. Obliczenia matematyczne
    radius_mm = ring_diameter_mm / 2.0
    total_length_mm = 0.0

    for i in range(1, len(sequence)):
        n1 = sequence[i-1]
        n2 = sequence[i]

        # Szukamy najkrótszej drogi między gwoździami po okręgu
        diff = abs(n1 - n2)
        if diff > nail_number / 2:
            diff = nail_number - diff

        # Kąt w radianach i długość cięciwy
        angle_rad = diff * (2.0 * math.pi / nail_number)
        distance = 2.0 * radius_mm * math.sin(angle_rad / 2.0)

        total_length_mm += distance
        total_length_mm += 10.0 # 10mm marginesu per gwóźdź na samo owinięcie

    # 3. Wynik
    total_meters = total_length_mm / 1000.0
    
    print("=" * 45)
    print(f"📊 RAPORT DLA: {os.path.basename(file_path)}")
    print("=" * 45)
    print(f"🔹 Liczba gwoździ na obręczy : {nail_number}")
    print(f"🔹 Długość sekwencji (ruchy): {len(sequence) - 1}")
    print(f"🔹 Średnica robota          : {ring_diameter_mm} mm")
    print(f"🧵 SZACUNKOWE ZUŻYCIE NICI  : {total_meters:.2f} metrów")
    print("=" * 45)

if __name__ == "__main__":
    # Inicjalizacja tkinter i ukrycie głównego (pustego) okna aplikacji
    root = tk.Tk()
    root.withdraw()
    
    print("Otwieranie systemowego okna wyboru pliku...")
    
    # Wywołanie systemowego explorera plików
    wybrany_plik = filedialog.askopenfilename(
        title="Wybierz plik wzoru dla robota (.gcode)",
        filetypes=[
            ("Pliki G-Code", "*.gcode"), 
            ("Pliki tekstowe", "*.txt"), 
            ("Wszystkie pliki", "*.*")
        ]
    )
    
    # Jeśli użytkownik wybrał plik (a nie kliknął "Anuluj")
    if wybrany_plik:
        # Możesz zmienić średnicę (np. 366.0) na sztywno poniżej, jeśli masz inny rozmiar
        calculate_string_length(wybrany_plik, ring_diameter_mm=260.0)
        
        # Zapobiega natychmiastowemu zamknięciu konsoli w Windowsie
        print("\nGotowe!")
        input("Naciśnij [Enter], aby zamknąć to okno...")
    else:
        print("Anulowano wybór pliku.")