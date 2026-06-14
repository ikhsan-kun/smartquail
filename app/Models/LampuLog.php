<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;

class LampuLog extends Model
{
    protected $table = 'lampu_log';

    protected $fillable = [
        'aksi',
        'triggered_by',
    ];

    const UPDATED_AT = null;
}
