#pragma once
#ifdef _WIN32
#include <sdkddkver.h>
#endif

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/bind_executor.hpp>

#include <memory>
#include <iostream>

#include "hotdog.h"
#include "result.h"

namespace net = boost::asio;
namespace sys = boost::system;
using std::literals::operator""s;

// Функция-обработчик операции приготовления хот-дога
using HotDogHandler = std::function<void(Result<HotDog> hot_dog)>;

class Order : public std::enable_shared_from_this<Order>{
public:
    Order(net::io_context& io, 
        int id, 
        HotDogHandler handler, 
        std::shared_ptr<GasCooker> gas_cooker,
        std::shared_ptr<Sausage> sausage, std::shared_ptr<Bread> bread) 
        :   io_{io}, 
            order_id_{id}, 
            handler_{std::move(handler)}, 
            gas_cooker_(std::move(gas_cooker)), 
            sausage_(std::move(sausage)), 
            bread_(std::move(bread)) {}

    void Execute() {
        BakeBread();
        FrySausage();
    }

private:
    void BakeBread() {
        bread_->StartBake(*gas_cooker_, [self = shared_from_this()]() {
            self->bread_timer_.expires_from_now(Milliseconds{1000});
            self->bread_timer_.async_wait(
            net::bind_executor(self->strand_, [self = std::move(self)](sys::error_code ec) {
                self->OnBaked(ec);
            }));
        });
    }

    void OnBaked(sys::error_code ec) {
        bread_->StopBaking();
        if (ec) {
            throw std::runtime_error("Fry error : "s + ec.what());
        } else {
            bread_baked_ = true;
        }
        CheckReadiness();
    }

    void FrySausage() {
        sausage_->StartFry(*gas_cooker_, [self = shared_from_this()]() {
            self->sausage_timer_.expires_from_now(Milliseconds{1500});
            self->sausage_timer_.async_wait(
            net::bind_executor(self->strand_, [self = std::move(self)](sys::error_code ec) {
                self->OnFried(ec);
            }));
        });
    }

    void OnFried(sys::error_code ec) {
        sausage_->StopFry();
        if (ec) {
            throw std::runtime_error("Fry error : "s + ec.what());
        } else {
            sausage_fried_ = true;
        }
        CheckReadiness();
    }

    void CheckReadiness() {
        if (delivered_) {
            return;
        }

        // Если все компоненты гамбургера готовы, упаковываем его
        if (IsReadyToDeliver()) {
            Deliver();
        }
    }

    void Deliver() {
        delivered_ = true;
        handler_(Result{HotDog{order_id_, sausage_, bread_}});
    }

    bool IsReadyToDeliver() const {
        return bread_baked_ && sausage_fried_ &&
               sausage_->IsCooked() && bread_->IsCooked();
    }

    int order_id_;
    net::io_context& io_;
    net::strand<net::io_context::executor_type> strand_{net::make_strand(io_)};

    std::shared_ptr<GasCooker> gas_cooker_;
    HotDogHandler handler_;

    std::shared_ptr<Sausage> sausage_; 
    std::shared_ptr<Bread> bread_;
    bool bread_baked_ = false;
    bool sausage_fried_ = false;
    bool delivered_ = false;

    net::steady_timer bread_timer_{io_, Milliseconds{1000}};
    net::steady_timer sausage_timer_{io_, Milliseconds{1500}};
};

// Класс "Кафетерий". Готовит хот-доги
class Cafeteria {
public:
    explicit Cafeteria(net::io_context& io)
        : io_{io} {
    }

    // Асинхронно готовит хот-дог и вызывает handler, как только хот-дог будет готов.
    // Этот метод может быть вызван из произвольного потока
    void OrderHotDog(HotDogHandler handler) {
        // TODO: Реализуйте метод самостоятельно
        // При необходимости реализуйте дополнительные классы
        const int order_id = ++next_order_id_;
        std::make_shared<Order>(
            io_, order_id, std::move(handler), gas_cooker_, store_.GetSausage(), store_.GetBread()
        )->Execute();
    }

private:
    net::io_context& io_;
    // Используется для создания ингредиентов хот-дога
    Store store_;
    // Газовая плита. По условию задачи в кафетерии есть только одна газовая плита на 8 горелок
    // Используйте её для приготовления ингредиентов хот-дога.
    // Плита создаётся с помощью make_shared, так как GasCooker унаследован от
    // enable_shared_from_this.
    std::shared_ptr<GasCooker> gas_cooker_ = std::make_shared<GasCooker>(io_);

    int next_order_id_ = 0;
};
