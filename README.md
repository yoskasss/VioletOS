# VioletOS 

**VioletOS**, C ve Assembly programlama dilleri ile yazılmış, x86 32-bit mimarisini hedefleyen, eğitim amaçlı ve zengin mor estetiğine sahip açık kaynaklı bir işletim sistemidir. 

Proje, hem klasik metin arayüzünü hem de modern grafik arayüzünü deneyimleyebilmeniz için **iki farklı sürüm** halinde sunulmaktadır:

1. **TUI Sürümü (Text User Interface / Metin Tabanlı v1.0):** Standart VGA 80x25 metin ekranı kullanan, klavye ile kontrol edilen, hızlı ve retro görünümlü işletim sistemi sürümüdür.
2. **GUI Sürümü (Graphical User Interface / Pencereli Grafik v2.0):** VBE (VESA BIOS Extensions) 800x600 çözünürlüğünde 32bpp doğrusal grafik moduna (linear framebuffer) sahip; pencere yöneticisi, çift arabellekli (double-buffered) çizim, masaüstü arka planı, görev çubuğu ve fare desteği sunan gelişmiş sürümdür.

---

## 🚀 Sürümlerin Karşılaştırması (TUI vs GUI)

Aşağıdaki tabloda VioletOS'in iki ana versiyonu arasındaki temel farklar özetlenmiştir:

| Özellik | TUI Sürümü (v1.0 - Klasik) | GUI Sürümü (v2.0 - Aktif) |
| :--- | :--- | :--- |
| **Ekran Modu** | VGA 80x25 Metin Modu (0xB8000) | VBE 800x600 32bpp Grafik Modu |
| **Kontrol Birimleri** | Sadece Klavye (Türkçe Q) | Klavye (Türkçe Q) + Fare (Mouse) |
| **Arayüz Elemanları** | ASCII Menüler, Klasik Terminal | Pencereler, Masaüstü Duvar Kağıdı, Görev Çubuğu |
| **Görüntü İşleme** | Doğrudan VGA Belleğine Karakter Yazımı | Arka Bellek (Backbuffer) ile Akıcı Grafik Çizimi |
| **Ağ/Tarayıcı Arayüzü**| Yok | Temel Grafiksel Web Tarayıcı Arayüzü |
| **Proje Dizini** | [TUI/](file:///home/enesaksoy/Masa%C3%BCst%C3%BC/VioletOS/TUI) | [GUI/](file:///home/enesaksoy/Masa%C3%BCst%C3%BC/VioletOS/GUI) (ve Proje Kök Dizini) |
| **ISO Çıktısı** | `build/violetosTUI.iso` | `build/violetosGUI.iso` |

---

## 🛠️ Sistem Gereksinimleri ve Bağımlılıklar

VioletOS'i derlemek, ISO oluşturmak ve test etmek için sisteminizde aşağıdaki geliştirme araçlarının kurulu olması gerekmektedir:

### Gerekli Paketler

| Araç | Kullanım Amacı |
| :--- | :--- |
| `gcc` (32-bit / multilib) | C kodlarını derlemek için x86_64 üzerinde 32-bit derleyici desteği |
| `nasm` | Assembly (boot ve kesme vektörleri) kodlarını derlemek için |
| `ld` | 32-bit ELF formatında linker |
| `grub-mkrescue` / `grub2-mkrescue` | Önyüklenebilir (bootable) ISO dosyası oluşturmak için |
| `xorriso` | GRUB'un ISO oluştururken kullandığı arka plan paketi |
| `qemu-system-i386` | İşletim sistemini bilgisayarınızda sanal olarak denemek için emülatör |

### Paket Kurulum Komutları

#### 💻 Debian / Ubuntu Tabanlı Dağıtımlar:
```bash
sudo apt-get update
sudo apt-get install gcc-multilib nasm make grub-pc-bin xorriso qemu-system-x86
```

#### 🎩 Fedora / RedHat Tabanlı Dağıtımlar:
```bash
sudo dnf install gcc nasm make grub2-tools xorriso qemu-system-x86 glibc-devel.i686 libgcc.i686
```

---

## ⚙️ Derleme ve Çalıştırma Kuralları

Her iki sürüm de bağımsız dizinlerde kendi derleme dosyalarına sahiptir. Hangi sürümü derlemek istiyorsanız ilgili klasöre geçiş yapmalısınız.

### 🖥️ 1. GUI Sürümünün Kurulumu (Grafiksel v2.0)

GUI kodları ana çalışma dizininde ve [GUI/](file:///home/enesaksoy/Masa%C3%BCst%C3%BC/VioletOS/GUI) klasöründe yer alır. Derlemek için ana klasörde terminali açın:

#### Derleme ve ISO Oluşturma
```bash
# build.sh dosyasını çalıştırarak derleme yapabilirsiniz:
chmod +x build.sh
./build.sh
```
Alternatif olarak doğrudan `make` kullanabilirsiniz:
```bash
make iso
```
*Üretilen Çıktı:* `build/violetos.iso`

#### QEMU ile Çalıştırma
```bash
make run
```

#### Kalıcı Disk Desteği ile Çalıştırma (Kayıt Yeteneği)
İşletim sisteminde yazdığınız dosyaların ve kodların bilgisayarı kapatsanız dahi silinmemesini (VioletFS kalıcılığı) istiyorsanız sanal disk imajı oluşturup çalıştırmalısınız:
```bash
make disk-image  # 64MB boyutunda boş disk imajı oluşturur (build/violetos.img)
make run-disk    # QEMU'yu disk sürücüsüyle birlikte başlatır
```

---

### ⌨️ 2. TUI Sürümünün Kurulumu (Metin Modu v1.0)

TUI sürümünü derlemek için öncelikle [TUI/](file:///home/enesaksoy/Masa%C3%BCst%C3%BC/VioletOS/TUI) dizinine geçiş yapmalısınız:

```bash
# Terminalde TUI klasörüne gidin
cd TUI
```

#### Derleme ve ISO Oluşturma
```bash
chmod +x build.sh
./build.sh
```
veya:
```bash
make iso
```
*Üretilen Çıktı:* `TUI/build/violetos.iso`

#### QEMU ile Çalıştırma
```bash
make run
```

#### Kalıcı Disk Desteği ile Çalıştırma
```bash
make disk-image
make run-disk
```

---

## 💾 Gerçek Donanımda Çalıştırma (USB Yazma)

VioletOS'i sanal makine dışında gerçek bir bilgisayarda önyüklemek (boot) isterseniz:

1. İlgili sürümün ISO dosyasını derleyin (`make iso`).
2. Bilgisayarınıza boş bir USB bellek takın (⚠️ **DİKKAT: USB bellekteki tüm veriler kalıcı olarak silinecektir!**).
3. Aşağıdaki komut yardımıyla ISO dosyasını USB'ye yazdırın:
   ```bash
   sudo dd if=build/violetos.iso of=/dev/sdX bs=4M status=progress conv=fsync
   ```
   *(Buradaki `/dev/sdX` kısmını kendi USB sürücünüzün adıyla değiştirin, örn: `/dev/sdb`. `lsblk` komutu ile kontrol edebilirsiniz!)*
4. Bilgisayarınızı yeniden başlatın, BIOS/Boot menüsüne girerek USB belleği seçin.
5. Bilgisayarınız yalnızca UEFI destekliyorsa, BIOS ayarlarından **Legacy/CSM Boot** modunu etkinleştirmeniz gerekmektedir.

---

## 📂 Proje Dizin Yapısı

```
VioletOS/
├── boot/           # Multiboot önyükleme (Assembly) ve GRUB yapılandırma dosyaları
├── kernel/         # Çekirdek temel kodları (GDT, IDT, Bellek Yönetimi, Kesmeler, Ana Metot)
├── drivers/        # Donanım sürücüleri (VGA ekran kartı, Klavye, PIT, ATA sabit disk, PCI, RTL8139 ağ kartı, Fare)
├── filesystem/     # VioletFS - Çekirdek içi basit ve düz dosya sistemi entegrasyonu
├── gui/            # Ana menü yönetimi, logo yerleşimi ve arayüz çizimleri
├── shell/          # Terminal arayüzü (Shell) ve basit not defteri (editor.c) uygulamaları
├── ide/            # Entegre Violet IDE kod düzenleyici yazılımı
├── violet_lang/    # Violet programlama dili için geliştirilmiş yorumlayıcı motoru
├── lib/            # Bellek ve metin (string) kütüphane fonksiyonları (libc benzeri)
├── include/        # Projenin başlık (.h) dosyaları
├── docs/           # Sistem mimarisi ve geliştirici dokümantasyonu (ARCHITECTURE.md)
├── site/           # Tanıtım web sayfası kodları ve hazır derlenmiş .iso / imaj dosyaları
├── Makefile        # Derleme ve QEMU çalıştırma otomasyonu
├── linker.ld       # Bellek segmentlerinin yerleşim şeması
└── README.md       # Bu bilgilendirme belgesi
```

---

## 🖥️ Ana Menü ve Navigasyon

Sistem açıldığında klavye yön tuşları ya da numara tuşları (`1`-`3`) ile kontrol edebileceğiniz bir ana menü sizi karşılar:

1. **Terminal:** Etkileşimli komut satırını başlatır.
2. **Violet IDE:** Betik kodlarınızı yazabileceğiniz metin editörünü ve derleyici arayüzünü açar.
3. **Shutdown:** APM/ACPI arabirimlerini tetikleyerek bilgisayarı veya emülatörü güvenli bir şekilde kapatır.

---

## 🐚 Terminal (Shell) Komutları

Terminal ekranında, donanım destekli yanıp sönen bir imleç eşliğinde komutlar yazabilirsiniz. Desteklenen komutlar şunlardır:

| Komut | Açıklama |
| :--- | :--- |
| `help` | Kullanılabilir tüm terminal komutlarını listeler. |
| `clear` | Terminal ekranını temizler. |
| `ls` | Dosya sistemindeki dosyaları ve klasörleri listeler. |
| `pwd` | Mevcut çalışma dizinini ekrana yazar. |
| `mkdir <dizin>` | Yeni bir dizin oluşturur. |
| `touch <dosya>` | Boş bir dosya oluşturur. |
| `rm <dosya>` | Belirtilen dosyayı siler. |
| `cat <dosya>` | Dosya içeriğini ekrana yazdırır. |
| `not <dosya>` | Basit metin editörünü açar (Kaydetmek için `Ctrl+O`, çıkmak için `Ctrl+Q`). |
| `ide` | Violet IDE geliştirme ortamını başlatır. |
| `run <dosya.v>` | Belirtilen Violet (`.v`) betik dosyasını yorumlayarak çalıştırır. |
| `fetch` | Sistem özelliklerini ve renk temasını gösteren bilgi kartı (Neofetch benzeri). |
| `reboot` | İşletim sistemini yeniden başlatır. |
| `shutdown` | Sistemi tamamen kapatır. |
| `echo <metin>` | Girilen metni ekrana yazdırır. |
| `date` | Sistemin açılışından itibaren geçen süreyi (uptime) gösterir. |
| `exit` | Terminalden çıkış yaparak ana menüye döner. |

---

## 📝 Violet Geliştirme Ortamı (IDE) ve Violet Programlama Dili

VioletOS, kendi geliştirdiğiniz programları doğrudan işletim sistemi üzerinde yazıp çalıştırabilmeniz için entegre bir **IDE** ve **Violet Dili yorumlayıcısı** barındırır. Betik dosyaları `.v` uzantısını kullanır.

### Violet IDE Kısayolları ve Kontrolleri
*   **Başlatma:** Ana menüden "Violet IDE" seçilerek ya da terminalde `ide` yazılarak açılır.
*   **`Ctrl + S`:** Mevcut dosyayı kaydeder.
*   **`Ctrl + R`:** Yazılan Violet kodunu derlemeden anında yorumlayıp çalıştırır.
*   **`Ctrl + Q`:** IDE'den çıkar (değişiklik varsa otomatik kaydeder).
*   **`:` (İki nokta üst üste):** Komut moduna geçer. Komut modundayken:
    *   `:open dosya.v` - Belirtilen dosyayı yükler.
    *   `:save` - Dosyayı kaydeder.
    *   `:new` - Yeni ve temiz bir dosya açar.
    *   `:run` - Kodu yorumlayıcıda çalıştırır.
*   **Yön Tuşları:** İmleci hareket ettirir.

### Violet Programlama Dili Örnek Kod Yapısı
Matematiksel işlemler, değişkenler, koşullar ve döngüler içeren bir örnek:

```v
// Violet Dil Örneği (.v)
print "Merhaba VioletOS Dunyasi!"

set isim "Enes"
set limit 5
print isim + " icin sayac baslatiliyor..."

set i 0
while i < limit
  print "Sayac degeri: " + i
  set i i + 1
end

if limit == 5
  print "Limit dogrulandi!"
else
  print "Limit hatali."
end
```

### Violet Dili Komutları ve Operatörleri

*   `print <ifade>`: Değerleri veya matematiksel ifadeleri ekrana yazdırır.
*   `set <degisken> <deger>`: Değişken tanımlar ve değer atar (örn: `set x 10`).
*   `input <degisken>`: Kullanıcıdan klavye girdisi alır ve uygun veri türüne çevirir.
*   `if <kosul> ... else ... end`: Koşullu dallanma.
*   `while <kosul> ... end`: Koşul doğru olduğu sürece çalışan döngü.
*   `loop <N> ... end`: Belirtilen N sayısı kadar döngüyü tekrar eder.
*   `clear`: Ekranı temizler.
*   `#` veya `//`: Satır içi yorum/açıklama satırı.
*   **Aritmetik Operatörler:** `+`, `-`, `*`, `/`
*   **Karşılaştırma Operatörleri:** `==`, `!=`, `>`, `<`, `>=`, `<=`
*   **Mantıksal Operatörler:** `and`, `or`, `not`

---

## 💜 Görsel Estetik ve Tasarım Sistemi

VioletOS, geliştiriciler için özel olarak seçilmiş **Mor/Eflatun** tabanlı modern bir renk paletine sahiptir:

*   **Ana Renkler:** Koyu arka plan üzerinde parlak eflatun/mor vurgular.
*   **Yazı Tipi & Okunabilirlik:** CRT monitör hissi veren, temiz görünümlü VGA karakterleri.
*   **IDE Aktif Satır:** Seçili satır mavi arka plan ve mor kılavuz kenarlığıyla vurgulanır.
*   **Görsel Arayüz (GUI Sürümü):** Çift ara bellek sayesinde pencereler sürüklenirken veya fare hareket ettirilirken titreme yapmaz, akıcı bir şekilde ekrana çizilir.

---

## 📄 Lisans ve Sürüm Bilgileri

*   **Lisans:** Eğitim Amaçlı Kullanım - VioletOS işletim sistemi geliştirme mantığını ve alt düzey mimariyi anlamak için bir öğrenme kaynağı olarak sunulmuştur.
*   **VioletOS Versiyonu:** 2.0.0 (GUI) / 1.0.0 (TUI)
*   **Geliştirici:** Enes Aksoy
