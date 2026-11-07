#pragma once
#include <cctype>
#include <ctime>
#include <mutex>
#include "board.h"
#include "http.h"
#include "mcp_server.h"

class PortugalWeather {
    friend struct PortugalWeatherTest;
    std::mutex mutex_;
    std::string days_[3];
    int64_t updated_[3] = {};
    struct City {
        int id;
        const char* name;
    };
    static std::string Fold(std::string value) {
        const char* accents[] = {"á", "à", "ã", "â", "é", "ê", "í", "ó",
                                 "ô", "õ", "ú", "ç", "Á", "É", "Ó"};
        const char* plain[] = {"a", "a", "a", "a", "e", "e", "i", "o",
                               "o", "o", "u", "c", "a", "e", "o"};
        for (size_t i = 0; i < 15; ++i) {
            size_t at = 0;
            while ((at = value.find(accents[i], at)) != std::string::npos)
                value.replace(at, 2, plain[i]);
        }
        for (auto& c : value)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return value;
    }
    static constexpr City cities_[] = {
        {1010500, "Aveiro"},
        {1020500, "Beja"},
        {1030300, "Braga"},
        {1030800, "Guimarães"},
        {1040200, "Bragança"},
        {1050200, "Castelo Branco"},
        {1060300, "Coimbra"},
        {1070500, "Évora"},
        {1080500, "Faro"},
        {1081505, "Sagres"},
        {1081100, "Portimão"},
        {1080800, "Loulé"},
        {1090700, "Guarda"},
        {1090821, "Penhas Douradas"},
        {1100900, "Leiria"},
        {1110600, "Lisboa"},
        {1121400, "Portalegre"},
        {1131200, "Porto"},
        {1141600, "Santarém"},
        {1151200, "Setúbal"},
        {1151300, "Sines"},
        {1160900, "Viana do Castelo"},
        {1171400, "Vila Real"},
        {1182300, "Viseu"},
        {2310300, "Funchal"},
        {2320100, "Porto Santo"},
        {3410100, "Vila do Porto"},
        {3420300, "Ponta Delgada"},
        {3430100, "Angra do Heroísmo"},
        {3440100, "Santa Cruz da Graciosa"},
        {3450200, "Velas"},
        {3460200, "Madalena"},
        {3470100, "Horta"},
        {3480200, "Santa Cruz das Flores"},
        {3490100, "Vila do Corvo"},
    };
    static const char* Description(int code) {
        switch (code) {
            case -99:
                return "---";
            case 0:
                return "Sem informação";
            case 1:
                return "Céu limpo";
            case 2:
                return "Céu pouco nublado";
            case 3:
                return "Céu parcialmente nublado";
            case 4:
                return "Céu muito nublado ou encoberto";
            case 5:
                return "Céu nublado por nuvens altas";
            case 6:
                return "Aguaceiros/chuva";
            case 7:
                return "Aguaceiros/chuva fracos";
            case 8:
                return "Aguaceiros/chuva fortes";
            case 9:
                return "Chuva/aguaceiros";
            case 10:
                return "Chuva fraca ou chuvisco";
            case 11:
                return "Chuva/aguaceiros forte";
            case 12:
                return "Períodos de chuva";
            case 13:
                return "Períodos de chuva fraca";
            case 14:
                return "Períodos de chuva forte";
            case 15:
                return "Chuvisco";
            case 16:
                return "Neblina";
            case 17:
                return "Nevoeiro ou nuvens baixas";
            case 18:
                return "Neve";
            case 19:
                return "Trovoada";
            case 20:
                return "Aguaceiros e possibilidade de trovoada";
            case 21:
                return "Granizo";
            case 22:
                return "Geada";
            case 23:
                return "Chuva e possibilidade de trovoada";
            case 24:
                return "Nebulosidade convectiva";
            case 25:
                return "Céu com períodos de muito nublado";
            case 26:
                return "Nevoeiro";
            case 27:
                return "Céu nublado";
            case 28:
                return "Aguaceiros de neve";
            case 29:
                return "Chuva e Neve";
            case 30:
                return "Chuva e Neve";
            default:
                return "Sem informação";
        }
    }
    void Refresh(int day) {
        auto http = Board::GetInstance().GetNetwork()->CreateHttp(4);
        if (!http)
            return;
        http->SetTimeout(6000);
        auto url =
            "https://api.ipma.pt/open-data/forecast/meteorology/cities/daily/"
            "hp-daily-forecast-day" +
            std::to_string(day) + ".json";
        if (!http->Open("GET", url))
            return;
        auto status = http->GetStatusCode();
        if (!status || *status != 200) {
            http->Close();
            return;
        }
        std::string body;
        char chunk[512];
        int64_t deadline = esp_timer_get_time() + 12000000;
        while (esp_timer_get_time() < deadline) {
            auto n = http->Read(chunk, sizeof(chunk));
            if (!n) {
                http->Close();
                return;
            }
            if (*n == 0)
                break;
            body.append(chunk, *n);
            if (body.size() > 16000) {
                http->Close();
                return;
            }
        }
        http->Close();
        CJsonUniquePtr json(cJSON_Parse(body.c_str()));
        if (!json || !cJSON_IsArray(cJSON_GetObjectItem(json.get(), "data")) ||
            !cJSON_IsString(cJSON_GetObjectItem(json.get(), "forecastDate")))
            return;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            days_[day] = std::move(body);
            updated_[day] = esp_timer_get_time();
        }
        ESP_LOGI("IPMA", "Forecast day %d ready", day);
    }

public:
    void Start() {
        BaseType_t ok = xTaskCreate(
            [](void* arg) {
                auto self = static_cast<PortugalWeather*>(arg);
                vTaskDelay(pdMS_TO_TICKS(15000));
                while (true) {
                    for (int i = 0; i < 3; ++i)
                        self->Refresh(i);
                    bool ready;
                    {
                        std::lock_guard<std::mutex> lock(self->mutex_);
                        ready = self->updated_[0] != 0;
                    }
                    vTaskDelay(pdMS_TO_TICKS(ready ? 900000 : 60000));
                }
            },
            "ipma", 6144, this, 1, nullptr);
        if (ok != pdPASS)
            ESP_LOGE("IPMA", "Cannot create refresh task");
    }
    ToolResult Get(const PropertyList& args) {
        auto city = args["city"].value<std::string>();
        int day = args["day"].value<int>();
        if (city.size() > 64)
            return std::unexpected("Nome de cidade demasiado longo");
        std::string body;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (updated_[day] == 0 || esp_timer_get_time() - updated_[day] > 1800000000LL)
                return std::unexpected(
                    "IPMA indisponível ou ainda a atualizar. Não inventes a previsão; tenta mais "
                    "tarde.");
            body = days_[day];
        }
        CJsonUniquePtr json(cJSON_Parse(body.c_str()));
        if (!json)
            return std::unexpected("Dados IPMA inválidos");
        auto date = cJSON_GetObjectItem(json.get(), "forecastDate");
        // Reject a cached feed for the wrong date, including across midnight.
        time_t requested = time(nullptr) + day * 86400;
        struct tm utc{};
        gmtime_r(&requested, &utc);
        char expected[11];
        strftime(expected, sizeof(expected), "%Y-%m-%d", &utc);
        if (!cJSON_IsString(date) || date->valuestring != std::string(expected))
            return std::unexpected(
                "A previsão IPMA recebida não corresponde à data pedida; não apresentar como "
                "atual.");
        CJsonUniquePtr result(cJSON_CreateObject());
        cJSON_AddStringToObject(result.get(), "source", "IPMA");
        cJSON_AddStringToObject(result.get(), "forecast_date", date->valuestring);
        cJSON_AddStringToObject(result.get(), "kind", "Previsão diária, não temperatura atual");
        auto output = cJSON_AddArrayToObject(result.get(), "locations");
        auto query = Fold(city);
        bool national = query == "portugal" || query.empty();
        for (const auto& place : cities_) {
            bool sample = place.id == 1131200 || place.id == 1110600 || place.id == 1080500 ||
                          place.id == 1060300 || place.id == 2310300 || place.id == 3420300;
            if (national ? !sample : Fold(place.name) != query)
                continue;
            cJSON* row = nullptr;
            cJSON_ArrayForEach (row, cJSON_GetObjectItem(json.get(), "data")) {
                auto id = cJSON_GetObjectItem(row, "globalIdLocal");
                if (!cJSON_IsNumber(id) || id->valueint != place.id)
                    continue;
                auto item = cJSON_CreateObject();
                cJSON_AddStringToObject(item, "city", place.name);
                for (auto key : {"tMin", "tMax", "precipitaProb", "predWindDir"}) {
                    auto field = cJSON_GetObjectItem(row, key);
                    if (field)
                        cJSON_AddItemToObject(item, key, cJSON_Duplicate(field, true));
                }
                auto code = cJSON_GetObjectItem(row, "idWeatherType");
                if (cJSON_IsNumber(code))
                    cJSON_AddStringToObject(item, "weather", Description(code->valueint));
                cJSON_AddItemToArray(output, item);
            }
        }
        if (cJSON_GetArraySize(output) == 0)
            return std::unexpected(
                "Localidade não disponível. Pede uma capital de distrito ou ilha, por exemplo "
                "Lisboa, Porto, Faro, Funchal ou Ponta Delgada.");
        if (national)
            cJSON_AddStringToObject(result.get(), "note",
                                    "Amostra de localidades; não é uma previsão uniforme de todo o "
                                    "país. Identifica as cidades ao resumir.");
        CJsonStringUniquePtr text(cJSON_PrintUnformatted(result.get()));
        return std::string(text.get());
    }
};
