# Serigrafi Makinesi - Arduino MEGA Projesi

Bu paket içinde Arduino MEGA (klon) ve RepRapDiscount 12864 için hazırlanmış proje dosyaları bulunmaktadır.

Dosyalar:
- serigrafi_makinesi_12864_full_menu_test.ino  -> Ana .ino dosyası (menü, test modu, hata kontrolleri)
- CHECKLIST.md                                -> Güvenlik & devreye alma kontrol listesi
- WIRING_TEMPLATE.txt                          -> Pin eşlemeleri ve yer tutucular
- manifest.txt                                 -> Dosya listesi

Nasıl kaydeder ve zip'lersin:
1. Yukarıdaki her dosya bloğunu ayrı dosya adıyla kaydet (örneğin serigrafi_makinesi_12864_full_menu_test.ino).
2. Tüm dosyaları bir klasöre koy (örn. SerigrafiProje).
3. ZIP oluştur:
   - Windows (PowerShell): Compress-Archive -Path .\SerigrafiProje\* -DestinationPath serigrafi_proje.zip
   - macOS / Linux: zip -r serigrafi_proje.zip SerigrafiProje/

Kullanım notları:
- Kod Arduino MEGA için yazıldı. Kütüphaneler:
  - U8g2
  - ClickEncoder
  - TimerOne
  - EEPROM (Arduino core ile birlikte)
- Ekran: RepRapDiscount Full Graphic 128x64 (ST7920). Eğer ekran farklı ise U8g2 constructor'u güncelleyin.
- Röle modüllerini test edin: IN pinleri aktif-LOW mi yoksa aktif-HIGH mı kontrol edin; gerekiyorsa kodda invert yapın.
- Solenoid bobin akımlarını paylaştığınızda checklist ve PSU önerisini güncelleyeceğim.

Eksik bilgiler (sonradan doldurulacak):
- Her solenoid valfin coil akımı (A) veya bobin direnci (ohm)
- Röle modülü tipi (optokuplör/JD-VCC var mı)
- Step motor modeli / sürücü tipi (A4988/DRV8825)

İleride yapılacaklar:
- Sen solenoid/ röle bilgilerini verdikten sonra PSU boyutlandırılması, koruma elemanları (TVS/RC/diyot) ve Fritzing şeması hazırlayacağım.
- İstersen ben bu dosyaları bir GitHub repo'suna da koyup ZIP oluşturabilirim; repo bilgilerini ver yeter.
