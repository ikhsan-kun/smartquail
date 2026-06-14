<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;

class SensorData extends Model
{
    protected $table = 'sensor_data';

    protected $fillable = [
        'suhu',
        'kelembapan',
        'amonia',
        'status',
        'kipas_active',
        'lampu_active',
        'sprayer_active',
    ];

    const UPDATED_AT = null;
}
