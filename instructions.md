# Budowanie
Install bazel from here: https://bazel.build/install/ubuntu

bazel build //...
(do tego być może musisz zainstalować inną wersję:
sudo apt update && sudo apt install bazel-7.6.1)

# VS Code Setup
- Skakanie po kodzie
- Błędy include'ów (żeby nie podkreślało)
- jak podkreśla nowo dodaną rzecz, to ponownie wygenerować compile_commands.json

### aby uzyskać (świeże) compile_commands.json
bazel run --config=compile-commands

### podpięcie tego compile_commands.json w VS Code
Ctrl+Shift+P ; wpisz C/C++: Edit Configurations (UI) ; scroll na dół ;
click Advanced Settings ; scroll tak do połowy ;
W compile commands wpisz ścieżkę do tego pliku, np
/home/glowacz/Documents/studia/bigtable-emulator/compile_commands.json

Ctrl+Shift+P ; Run "C/C++: Reset IntelliSense Database"

# Uruchomienie
U mnie zadziałało

./bazel-bin/emulator

Ale nie jestem w stanie podać portu


# Instalacja Google Cloud CLI 
(żeby wysyłać requesty zgodne z Bigtable API)

sudo apt-get update
sudo apt-get install apt-transport-https ca-certificates gnupg curl

curl https://packages.cloud.google.com/apt/doc/apt-key.gpg | sudo gpg --dearmor -o /usr/share/keyrings/cloud.google.gpg

echo "deb [signed-by=/usr/share/keyrings/cloud.google.gpg] https://packages.cloud.google.com/apt cloud-sdk main" | sudo tee -a /etc/apt/sources.list.d/google-cloud-sdk.list

sudo apt-get update && sudo apt-get install google-cloud-cli

gcloud init # (zaloguj się, wybierz losowy projekt)

gcloud components update # mi pisało, że coś tam jest wyłączone, ale dało inną komendę, którą trzeba wykonać
gcloud components install cbt # tak samo

# Wysyłanie requestów do aplikacji

### Za każdym otwarciem terminala, z którego chcemy wysyłać requesty
export BIGTABLE_EMULATOR_HOST=localhost:8888

Można wysyłać komendy cbt, na przykład

### create
cbt -project=p -instance=i createtable persistent-table
cbt -project=p -instance=i createfamily persistent-table cf

### set GC policy
cbt -project=p -instance=i setgcpolicy persistent-table cf maxversions=2
// dodanie na końcu force spowoduje, że policy zostanie na pewno i natychmiast zastosowana, nawet jeśli mieliśmy więcej wersji
// bez force trzeba trochę poczekać aż się uruchomi GC, a jeśli dajemy bardziej restrykcyjną policy niż była to może wgl nie zadziałać

### write
cbt -project=p -instance=i set persistent-table row1 cf:col1=saved_data

### read
cbt -project=p -instance=i read persistent-table

// tylko wybrane wiersze
cbt -project=p -instance=i read persistent-table regex='row1|row3'

// tylko 1 wiersz
cbt -project=p -instance=i lookup persistent-table row1

cbt -project=p -instance=i lookup persistent-table row1 columns=cf:col1


### delete table (and its cfs)
cbt -project=p -instance=i deletetable persistent-table

### others
cbt --help

# Zapełnianie bazy skryptem
// można odkomentować i zakomentować odpowiednie jego części, w zależności od potrzeb
// np ja wykryłem, że jest problem przy wielu column families
`chmod +x populate_db.sh`
`./populate_db.sh`

# Korzystanie z RocksDB
Jest w commicie: 
- trzeba pozmieniać/pododawać rzeczy do .bazelrc, MODULE.bazel, BUILD.bazel
- oraz w emulator.cc, bo to jest entry point tego projektu i tutaj używamy tego POC kodu
- POC kod korzystający z RocksDB jest w plikach storage_engine.cc i storage_engine.h

# Dodawanie headerów
- dodajemy do bigtable_emulator_common.bzl nazwę headera i src tak jak storage.h

# Czyszczenie całej bazy
`bazel run //:clear`