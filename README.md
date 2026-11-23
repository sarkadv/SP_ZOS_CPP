# Virtuální souborový systém založený na i-uzlech

Souborový systém založený na i-uzlech, který je celý uložený v jednom souboru na disku, s interaktivním
příkazovým řádkem pro práci se soubory. Napsáno v C. Semestrální práce z předmětu KIV/ZOS.

## Struktura souborového systému

Každý soubor (obyčejný i adresář) je reprezentován i-uzlem. I-uzel obsahuje velikost souboru, počet referencí
a odkazy na datové bloky: 5 přímých odkazů, jeden nepřímý odkaz 1. úrovně a jeden nepřímý odkaz 2. úrovně.

Soubor se souborovým systémem se skládá z:

- **superbloku** s informacemi o souborovém systému,
- **bitmap** volných i-uzlů a datových bloků,
- **i-uzlů** (1000),
- **datových bloků** o velikosti 1000 bajtů. Do jednoho bloku se vejde 250 odkazů na další bloky.

Adresář obsahuje dvojice (název souboru, číslo i-uzlu) a všechny jeho položky se musí vejít do jednoho
datového bloku. Po načtení souborového systému si program rekurzivně sestaví adresářový strom v paměti.

Po každém příkazu, který souborový systém mění, se změny hned zapíšou do souboru na disku.

### Omezení

- Nejvýše 1000 souborů najednou (podle počtu i-uzlů).
- Počet položek v adresáři je omezen velikostí jednoho datového bloku.
- Název souboru může mít nejvýše 12 znaků.
- Minimální velikost souborového systému je 50 kB, protože přibližně 40 kB zaberou jeho struktury.

## Použití

```
./vfs <název souborového systému>
```

Pokud soubor ještě neexistuje, vytvořte ho nejdříve příkazem `format`, např. `format 600MB` (podporované
jednotky jsou `B`, `KB`, `MB` a `GB`). Poté zadávejte příkazy. Cesty mohou být absolutní i relativní a mohou
obsahovat `.` a `..`.

| Příkaz                            | Popis                                                                      |
| --------------------------------- | -------------------------------------------------------------------------- |
| `format <velikost>`               | Naformátuje soubor na souborový systém dané velikosti (přemaže stará data) |
| `mkdir <adresář>`                 | Vytvoří adresář                                                            |
| `rmdir <adresář>`                 | Smaže prázdný adresář                                                      |
| `ls [adresář]`                    | Vypíše obsah adresáře (bez parametru aktuálního)                           |
| `cd <adresář>`                    | Změní aktuální adresář                                                     |
| `pwd`                             | Vypíše aktuální cestu                                                      |
| `incp <soubor na disku> <cesta>`  | Nahraje soubor z pevného disku do souborového systému                      |
| `outcp <soubor> <cesta na disku>` | Nahraje soubor ze souborového systému na pevný disk                        |
| `cat <soubor>`                    | Vypíše obsah souboru                                                       |
| `cp <soubor> <cesta>`             | Zkopíruje soubor                                                           |
| `mv <soubor> <cesta nebo název>`  | Přesune nebo přejmenuje soubor                                             |
| `rm <soubor>`                     | Smaže soubor                                                               |
| `slink <soubor> <odkaz>`          | Vytvoří symbolický odkaz na soubor                                         |
| `info <soubor nebo adresář>`      | Vypíše název, velikost, číslo i-uzlu a přímé a nepřímé odkazy              |
| `load <soubor na disku>`          | Provede příkazy ze souboru na disku, jeden příkaz na řádek                 |
| `help`                            | Vypíše všechny příkazy                                                     |
| `end`                             | Ukončí program                                                             |

Symbolický odkaz je běžný soubor s jedním datovým blokem, ve kterém je uložena absolutní cesta k odkazovanému
souboru. Dá se s ním pracovat jako s původním souborem, např. `cat` vypíše obsah odkazovaného souboru. Odkazy
lze řetězit. Když se odkazovaný soubor smaže, odkaz přestane fungovat.

## Moduly programu

| Modul        | Účel                                                                                  |
| ------------ | ------------------------------------------------------------------------------------- |
| `vfs`        | Vstupní bod: vytvoří strukturu souborového systému a ve smyčce vykonává příkazy       |
| `superblock` | Superblok s informacemi o souborovém systému                                          |
| `inode`      | I-uzel                                                                                |
| `data_block` | Datový blok                                                                           |
| `directory`  | Položky adresáře (název ↔ i-uzel) a adresářový strom                                  |
| `bitmap`     | Bitmapy volných i-uzlů a datových bloků                                               |
| `parser`     | Zpracování vstupu od uživatele a hledání cíle zadané cesty                            |
| `commands`   | Jednotlivé příkazy                                                                    |
