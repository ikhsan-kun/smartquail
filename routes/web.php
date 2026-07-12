<?php

use App\Http\Controllers\SensorController;
use Illuminate\Support\Facades\Route;

// Dashboard langsung tanpa login
Route::get('/', function () {
    return view('dashboard');
})->name('dashboard');

// Public ESP8266 API routes (no authentication required for IoT devices)
Route::post('/api/esp/sensor', [SensorController::class, 'espStoreSensor'])->name('api.esp.sensor');
Route::get('/api/esp/sprayer-status', [SensorController::class, 'espSprayerStatus'])->name('api.esp.sprayer-status');

// API Telemetry & Controls (public, no auth needed)
Route::get('/api/sensor/latest', [SensorController::class, 'latest'])->name('api.sensor.latest');
Route::post('/api/sprayer/toggle', [SensorController::class, 'toggleSprayer'])->name('api.sprayer.toggle');
Route::post('/api/kipas/toggle', [SensorController::class, 'toggleKipas'])->name('api.kipas.toggle');
Route::post('/api/lampu/toggle', [SensorController::class, 'toggleLampu'])->name('api.lampu.toggle');
