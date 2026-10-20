import math

print("===================== Kalkulačka =====================")
print("Můžeš psát běžné příklady, např.: 5+3*2")
print("Pro mocniny napiš: pow(číslo, mocnina) např. pow(2,3)")
print("Pro druhou odmocninu napiš: sqrt(číslo) např. sqrt(9)")
print("Pro n-tou odmocninu napiš: root(číslo, n) např. root(27,3)")
print("Pro procenta napiš: 200*50% (50% z 200) nebo 100+20% (přičíst 20%)")
print("Dostupné operace: +, -, *, /, pow(), sqrt(), root(), %")
print("Pro konec napiš: exit nebo konec")
print("================================================================")

def root(x, n):
    """Vrátí n-tou odmocninu z čísla x."""
    return x ** (1 / n)

def parse_percent(expr):
    """Převede výrazy s procenty na správný matematický zápis."""
    # Projdeme výraz odzadu a zpracujeme každé %
    while '%' in expr:
        pos = expr.find('%')
        
        # Najdi číslo před % (procento)
        end = pos - 1
        start = end
        while start >= 0 and (expr[start].isdigit() or expr[start] == '.'):
            start -= 1
        start += 1
        percent = expr[start:end+1]
        
        # Najdi operátor před procentem
        op_pos = start - 1
        while op_pos >= 0 and expr[op_pos] == ' ':
            op_pos -= 1
        
        if op_pos >= 0 and expr[op_pos] in ['+', '-', '*', '/']:
            operator = expr[op_pos]
            
            # Najdi číslo před operátorem
            num_end = op_pos - 1
            while num_end >= 0 and expr[num_end] == ' ':
                num_end -= 1
            num_start = num_end
            while num_start >= 0 and (expr[num_start].isdigit() or expr[num_start] == '.'):
                num_start -= 1
            num_start += 1
            base = expr[num_start:num_end+1]
            
            # Vytvoř náhradu podle operátoru
            if operator == '+':
                replacement = base + "*(1+" + percent + "/100)"
            elif operator == '-':
                replacement = base + "*(1-" + percent + "/100)"
            elif operator == '*':
                replacement = base + "*(" + percent + "/100)"
            else:  # operator == '/'
                replacement = base + "/(" + percent + "/100)"
            
            # Nahraď celý výraz
            expr = expr[:num_start] + replacement + expr[pos+1:]
        else:
            # Jen nahraď % za /100
            expr = expr[:start] + "(" + percent + "/100)" + expr[pos+1:]
    
    return expr

# Hlavní smyčka
while True:
    try:
        expr = input("Zadej příklad: ")
        if expr.lower() in ["exit", "quit", "konec"]:
            print("Konec programu.")
            break
        # Zpracuj procenta
        expr = parse_percent(expr)
        expr = expr.replace("sqrt", "math.sqrt")
        # eval vyhodnotí výraz a použije funkce pow, math.sqrt a root
        result = eval(expr, {"math": math, "pow": pow, "root": root})
        print("Výsledek:", result)
    except Exception as e:
        print("Chyba:", e)
