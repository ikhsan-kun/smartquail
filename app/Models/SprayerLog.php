<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;

class SprayerLog extends Model
{
    protected $table = 'sprayer_log';

    protected $fillable = [
        'aksi',
        'triggered_by',
    ];

    const UPDATED_AT = null;
}
