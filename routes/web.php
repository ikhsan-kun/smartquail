<?php

use App\Http\Controllers\AuthController;
use App\Http\Controllers\SensorController;
use Illuminate\Support\Facades\Route;

// Guest routes (redirect to dashboard if already logged in)
Route::middleware('guest')->group(function () {
    Route::get('/', [AuthController::class, 'showLogin']);
    Route::get('/login', [AuthController::class, 'showLogin'])->name('login');
    Route::post('/login', [AuthController::class, 'login'])->name('login.submit');
});

// Public ESP8266 API routes (no authentication required for IoT devices)
Route::post('/api/esp/sensor', [SensorController::class, 'espStoreSensor'])->name('api.esp.sensor');
Route::get('/api/esp/sprayer-status', [SensorController::class, 'espSprayerStatus'])->name('api.esp.sprayer-status');

// Authenticated routes
Route::middleware('auth')->group(function () {
    Route::get('/dashboard', function () {
        return view('dashboard');
    })->name('dashboard');

    Route::post('/logout', [AuthController::class, 'logout'])->name('logout');

    // API Telemetry & Controls
    Route::get('/api/sensor/latest', [SensorController::class, 'latest'])->name('api.sensor.latest');
    Route::post('/api/sprayer/toggle', [SensorController::class, 'toggleSprayer'])->name('api.sprayer.toggle');
    Route::post('/api/kipas/toggle', [SensorController::class, 'toggleKipas'])->name('api.kipas.toggle');
    Route::post('/api/lampu/toggle', [SensorController::class, 'toggleLampu'])->name('api.lampu.toggle');
});
