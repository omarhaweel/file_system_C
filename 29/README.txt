LESING AV MASTER-FILE-TABLE:
1. En peker settes til starten paa et buffer.
2. En offsett blir deklarert som hopper antall bytes som vi oonsker aa lese for
en bestemt variabel.
3. Etter at vi har lest inn alle variablene for en inode saa gjentar vi dette i
form av en lokke.
4. Alle inoder blir lagret i et array som holder paa alle inodepekere.
5. Deretter looper vi gjennom alle inodene og setter pekerne til hverandre

IMPLEMENTASJONSKRAV:
- Saa vidt vi ser saa har vi forventet output ifoolge testene, og at det
ikke er noen memory leaks

AVVIK I IMPLEMENTASJON:
- Vi bruker en static variabel num_inodes for aa sette pekere til de unike id-ene

TESTER:
- Saa vidt vi kan se saa fungerer alle testene, men at vi allokerer og freer
flere ganger enn sammenligningsfila gjorde
