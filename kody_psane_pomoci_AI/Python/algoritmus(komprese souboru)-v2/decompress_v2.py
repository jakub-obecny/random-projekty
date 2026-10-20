"""
Dekompresní skript pro PC - verze s více dávkami
Dekomprimuje všechny dávky z jednoho .dat souboru do jednoho velkého .txt souboru
"""
import os

def nacti_davky(cesta):
    """Načte všechny dávky z .dat souboru"""
    davky = []
    aktualni_davka = {}
    
    with open(cesta, 'r', encoding='utf-8') as f:
        for radek in f:
            radek = radek.strip()
            if radek == "# DAVKA_START":
                aktualni_davka = {}
            elif radek == "# DAVKA_END":
                if aktualni_davka:
                    davky.append(aktualni_davka)
            elif '=' in radek and not radek.startswith('#'):
                key, value = radek.split('=', 1)
                aktualni_davka[key] = int(value)
    
    return davky

def dekomprimuj(cesta_ke_komprimovanemu):
    try:
        print(f"Načítám soubor: {cesta_ke_komprimovanemu}")
        
        davky = nacti_davky(cesta_ke_komprimovanemu)
        
        if not davky:
            print("CHYBA: Žádné dávky nenalezeny!")
            return False
        
        print(f"Nalezeno {len(davky)} dávek:")
        celkem_radku = 0
        for d in davky:
            print(f"  Dávka {d['DAVKA']}: {d['LINE_COUNT']:,} řádků")
            celkem_radku += d['LINE_COUNT']
        print(f"  Celkem: {celkem_radku:,} řádků")
        
        # Výstupní soubor
        vystupni_soubor = cesta_ke_komprimovanemu.replace('.dat', '_decompressed.txt')
        if vystupni_soubor == cesta_ke_komprimovanemu:
            vystupni_soubor = cesta_ke_komprimovanemu + '_decompressed.txt'
        
        print(f"\nZahajuji dekompresci do: {vystupni_soubor}")
        print("Toto může chvíli trvat...\n")
        
        zpracovano_celkem = 0
        
        with open(vystupni_soubor, 'w', encoding='utf-8') as f_out:
            for davka in davky:
                print(f"Zpracovávám dávku {davka['DAVKA']}/{len(davky)}...")
                
                x = davka['START_X']
                inkrement = davka['INCREMENT']
                konstanta = davka['CONSTANT']
                pocet_radku = davka['LINE_COUNT']
                
                for i in range(pocet_radku):
                    vysledek = konstanta * x
                    f_out.write(f"Číslo krát {x} = {vysledek}\n")
                    x += inkrement
                    
                    zpracovano_celkem += 1
                    if zpracovano_celkem % 1000 == 0:
                        procent = (zpracovano_celkem / celkem_radku) * 100
                        print(f"  Celkový průběh: {zpracovano_celkem:,}/{celkem_radku:,} řádků ({procent:.1f}%)")
                
                print(f"  Dávka {davka['DAVKA']} hotova!")
        
        # Statistiky
        velikost_compressed = os.path.getsize(cesta_ke_komprimovanemu)
        velikost_decompressed = os.path.getsize(vystupni_soubor)
        
        print(f"\n{'=' * 50}")
        print(f"DEKOMPRESE DOKONČENA!")
        print(f"{'=' * 50}")
        print(f"Výstupní soubor:     {vystupni_soubor}")
        print(f"Komprimovaný:        {velikost_compressed:,} bytů ({velikost_compressed/1024:.2f} KB)")
        print(f"Dekomprimovaný:      {velikost_decompressed:,} bytů ({velikost_decompressed/1024/1024:.2f} MB)")
        print(f"Kompresní poměr:     {(1 - velikost_compressed/velikost_decompressed)*100:.4f}%")
        print(f"Celkem řádků:        {celkem_radku:,}")
        print(f"Celkem dávek:        {len(davky)}")
        
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
    print("=" * 50)
    print("DEKOMPRESNÍ NÁSTROJ - MULTI-BATCH VERZE")
    print("=" * 50)
    print()
    
    cesta = input("Zadej cestu k souboru .dat: ").strip().strip('"').strip("'")
    print()
    
    uspech = dekomprimuj(cesta)
    
    print()
    if uspech:
        print("Hotovo! Dekomprimovaný soubor je připraven.")
    else:
        print("Dekomprese se nezdařila.")
    
    print()
    input("Stiskni Enter pro ukončení...")