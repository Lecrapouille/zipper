# Basic standalone application

To compile the standalone unzipper demo:

```
cd Unzipper
make -j8
./build/demo_unzip -p -f -o /tmp ../../../tests/issues/issue_05_password.zip
```

Type `1234` as password in the prompt and for this given example.
Beware password are displayed on the console.

Commande line:
- `-p` for asking you to type a password in the case your zip file has been encrypted.
- `-f` to force smashing files when extracting.
- `-o` path to extract data, if not given the zip is extracted at `.`.
- `file.zip` shall be given at the last position of the command line.
- `-h` display the help.
