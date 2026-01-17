# Budowanie
Install bazel from here: https://bazel.build/install/ubuntu

bazel build //...
(do tego być może musisz zainstalować inną wersję:
sudo apt update && sudo apt install bazel-7.6.1)

# Skakanie po kodzie
bazel run --config=compile-commands

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
export BIGTABLE_EMULATOR_HOST=localhost:8888

Można wysyłać komendy cbt, na przykład

### write
cbt -project=p -instance=i createtable persistent-table
cbt -project=p -instance=i createfamily persistent-table cf
cbt -project=p -instance=i set persistent-table row1 cf:col1=saved_data

### read
cbt -project=p -instance=i read persistent-table


# Korzystanie z RocksDB
Jest w commicie: 
- trzeba pozmieniać/pododawać rzeczy do .bazelrc, MODULE.bazel, BUILD.bazel
- oraz w emulator.cc, bo to jest entry point tego projektu i tutaj używamy tego POC kodu
- POC kod korzystający z RocksDB jest w plikach storage_engine.cc i storage_engine.h