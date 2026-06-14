<?php

namespace Database\Seeders;

use Illuminate\Database\Console\Seeds\WithoutModelEvents;
use Illuminate\Database\Seeder;

use App\Models\SensorData;
use Illuminate\Support\Carbon;

class SensorDataSeeder extends Seeder
{
    /**
     * Run the database seeds.
     */
    public function run(): void
    {
        $now = Carbon::now();

        for ($i = 19; $i >= 0; $i--) {
            // Generate realistic values
            $suhu = round(rand(240, 380) / 10, 1); // 24.0 - 38.0
            $kelembapan = round(rand(500, 950) / 10, 1); // 50.0 - 95.0
            $amonia = round(rand(30, 250) / 10, 1); // 3.0 - 25.0

            // Determine status based on thresholds
            if ($suhu > 35.0 || $amonia > 20.0) {
                $status = 'bahaya';
            } elseif ($suhu > 33.0 || $amonia > 15.0) {
                $status = 'waspada';
            } else {
                $status = 'normal';
            }

            SensorData::create([
                'suhu' => $suhu,
                'kelembapan' => $kelembapan,
                'amonia' => $amonia,
                'status' => $status,
                'created_at' => $now->copy()->subHours($i),
            ]);
        }
    }
}
