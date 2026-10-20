"""
Dekompresní skript pro PC
Tento skript dekomprimuje soubor vytvořený na Raspberry Pi Pico
"""

def dekomprimuj_soubor(cesta_ke_komprimovanemu):
    """
    Dekomprimuje soubor vytvořený vlastním kompresním algoritmem
    """
    try:
        print(f"Načítám komprimovaný soubor: {cesta_ke_komprimovanemu}")
        
        # Čtení metadat z hlavičky
        metadata = {}
        with open(cesta_ke_komprimovanemu, 'r', encoding='utf-8') as f:
            line = f.readline()
            if line.strip() != "# CUSTOM_COMPRESSION_V1":
                print("CHYBA: Neplatný formát souboru!")
                return False
            
            # Čtení metadat
            while True:
                line = f.readline().strip()
                if line == "# END_HEADER":
                    break
                if '=' in line:
                    key, value = line.split('=', 1)
                    metadata[key] = int(value)
        
        print(f"\nNalezená metadata:")
        print(f"  Počáteční X: {metadata['START_X']}")
        print(f"  Inkrement: {metadata['INCREMENT']}")
        print(f"  Konstanta: {metadata['CONSTANT']}")
        print(f"  Počet řádků: {metadata['LINE_COUNT']}")
        
        # Generování výstupního názvu
        vystupni_soubor = cesta_ke_komprimovanemu.replace('_compressed.dat', '_decompressed.txt')
        if vystupni_soubor == cesta_ke_komprimovanemu:
            vystupni_soubor = cesta_ke_komprimovanemu + '_decompressed.txt'
        
        print(f"\nZahajuji dekompresci...")
        print(f"Výstupní soubor: {vystupni_soubor}")
        
        # Rekonstrukce původních dat
        x = metadata['START_X']
        inkrement = metadata['INCREMENT']
        konstanta = metadata['CONSTANT']
        pocet_radku = metadata['LINE_COUNT']
        
        with open(vystupni_soubor, 'w', encoding='utf-8') as f_out:
            for i in range(pocet_radku):
                vysledek = konstanta * x
                radek = f"Číslo krát {x} = {vysledek}\n"
                f_out.write(radek)
                
                x += inkrement
                
                # Zobrazení průběhu
                if (i + 1) % 1000 == 0:
                    procent = ((i + 1) / pocet_radku) * 100
                    print(f"  Zpracováno: {i + 1}/{pocet_radku} řádků ({procent:.1f}%)")
        
        print(f"\n✓ Dekomprese dokončena!")
        print(f"✓ Vytvořen soubor: {vystupni_soubor}")
        
        # Zobrazení statistik
        import os
        velikost_compressed = os.path.getsize(cesta_ke_komprimovanemu)
        velikost_decompressed = os.path.getsize(vystupni_soubor)
        
        print(f"\nStatistiky:")
        print(f"  Komprimovaný soubor: {velikost_compressed:,} bytů ({velikost_compressed/1024:.2f} KB)")
        print(f"  Dekomprimovaný soubor: {velikost_decompressed:,} bytů ({velikost_decompressed/1024:.2f} KB)")
        print(f"  Kompresní poměr: {(1 - velikost_compressed/velikost_decompressed)*100:.2f}%")
        
        return True
        
    except FileNotFoundError:
        print(f"CHYBA: Soubor '{cesta_ke_komprimovanemu}' nebyl nalezen!")
        return False
    except Exception as e:
        print(f"CHYBA při dekompresci: {e}")
        import traceback
        traceback.print_exc()
        return False


if __name__ == "__main__":
    print("=" * 60)
    print("DEKOMPRESNÍ NÁSTROJ PRO RASPBERRY PI PICO VÝSTUPY")
    print("=" * 60)
    print()
    
    # Požádání uživatele o cestu k souboru
    cesta = input("Zadej cestu ke komprimovanému souboru (.dat): ").strip()
    
    # Odstranění případných uvozovek (pokud uživatel zkopíruje cestu z průzkumníka)
    cesta = cesta.strip('"').strip("'")
    
    print()
    
    # Dekomprese
    uspech = dekomprimuj_soubor(cesta)
    
    print()
    if uspech:
        print("✓ Hotovo! Dekomprimovaný soubor je připraven k použití.")
    else:
        print("✗ Dekomprese se nezdařila.")
    
    print()
    input("Stiskni Enter pro ukončení...")
