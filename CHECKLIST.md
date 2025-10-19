--- Serigrafi Makinesi - Devreye Alma Checklist ---

1) Donanım hazırlığı
- [ ] Arduino MEGA ve USB bağlantısı hazır.
- [ ] 12864 ekran + enkoder takılı, ekran aydınlanıyor.
- [ ] Step sürücü (A4988/DRV8825) doğru şekilde bağlandı ve VMOT harici 12V ile beslendi.
- [ ] Röle modülleri için 5V besleme hazır.
- [ ] Solenoidler bağlı değilken bobin dirençleri ölçüldü (ohm).
- [ ] Tüm GND'ler ortaklandı (Arduino, step sürücü, 12V PSU, röle modülleri).

2) Güvenlik donanımı
- [ ] E-STOP butonu planlandı ve erişilebilir.
- [ ] 12V PSU uygun akım kapasitesinde ve sigortalı.
- [ ] Flyback/TVS/RC koruması için plan yapıldı.

3) İlk yazılım testi (güçsüz solenoid)
- [ ] Sadece Arduino + LCD + enkoder ile ana menü çalıştırıldı.
- [ ] Test Modu ile sensörleri okuma testi yapıldı (sensörleri elle tetikleyerek 1/0 kontrolü).
- [ ] Test Modu ile step motor sürücü düşük adım gönderimi kontrol edildi (sürücü enable kapalı iken dikkat).
- [ ] Röle modüllerinin IN pinleri elle tetiklenerek LED/rele klik sesi kontrol edildi.

4) Güçlü test (tek solenoid)
- [ ] Tek solenoid röle üzerinden bağlandı, 12V PSU ile kısa süreli açma/kapatma testi yapıldı.
- [ ] Solenoid akımı ölçüldü; röle kontakları bu akımı destekliyor mu kontrol edildi.

5) Entegrasyon testi (tam döngü)
- [ ] Tüm sensörlerin doğru polaritesi sağlandı.
- [ ] Otomatik mod düşük adım/uzun süre ile test edildi.
- [ ] Hata durumları (limit sensör ulaşmama, piston sensör hatası) test edildi.

6) Son kontrol
- [ ] E-STOP fonksiyonu test edildi.
- [ ] PSU sigortaları uygun değerlere ayarlandı.

NOT: Solenoid bobinlerine uygun koruma elemanları ve doğru güç kaynağı seçimi kritik. Eksik bilgiler geldikten sonra PSU ve koruma tavsiyesi güncellenecek.
