#include "mbed.h"
#include <string>

// Пин конфигурация
AnalogIn ecg_signal(A5);  // Вход ЭКГ (A0)
BufferedSerial pc(USBTX, USBRX, 115200);  // UART

// Параметры детекции
const int P_WAVE_THRESH = 300;       // Порог P-зубца (mV)
const int QRS_THRESH = 500;          // Порог QRS (mV)
const int T_WAVE_THRESH = 250;       // Порог T-зубца (mV)
const int REFRACTORY_PERIOD = 200;   // Рефрактерный период (ms)

// Глобальные переменные
Timer ecg_timer;
int last_qrs_time = 0;
bool qrs_detected = false;

// Фильтр для устранения шума
int apply_filter(int sample) {
    static int buf[5] = {0};
    static int index = 0;

    buf[index] = sample;
    index = (index + 1) % 5;

    std::sort(buf, buf + 5);  // Медианный фильтр
    return buf[2];
}

int main() {
    ecg_timer.start();

    printf("ECG Monitoring Started (Numerical Mode)\r\n");
    printf("Sampling rate: 100 Hz\r\n");
    printf("Thresholds: P>%dmV, QRS>%dmV, T>%dmV, Refractory: %dms\r\n\r\n",
           P_WAVE_THRESH, QRS_THRESH, T_WAVE_THRESH, REFRACTORY_PERIOD);

    while (1) {
        // Чтение и фильтрация сигнала
        int raw_ecg = ecg_signal.read_u16() * 3300 / 65535;  // Конвертация в mV (0–3300)
        int filtered_ecg = apply_filter(raw_ecg);
        int current_time = ecg_timer.elapsed_time().count();

        // Вывод сырых данных и состояния
        printf("Time: %5d ms | ECG: %4d mV | ", current_time, filtered_ecg);

        // Детекция событий
        if (filtered_ecg > QRS_THRESH && (current_time - last_qrs_time) > REFRACTORY_PERIOD) {
            last_qrs_time = current_time;
            qrs_detected = true;
            printf("QRS DETECTED (Peak: %d mV)\r\n", filtered_ecg);
        } else if (filtered_ecg > P_WAVE_THRESH && !qrs_detected) {
            printf("P-WAVE (Peak: %d mV)\r\n", filtered_ecg);
        } else if (qrs_detected && filtered_ecg > T_WAVE_THRESH && (current_time - last_qrs_time) > 100) {
            qrs_detected = false;
            printf("T-WAVE (Peak: %d mV)\r\n", filtered_ecg);
        } else {
            printf("Baseline\r\n");
        }

        ThisThread::sleep_for(10ms);  // 100 Hz sampling
    }
}
