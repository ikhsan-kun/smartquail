<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;

class KipasLog extends Model
{
    protected $table = 'kipas_log';

    protected $fillable = [
        'aksi',
        'triggered_by',
    ];

    const UPDATED_AT = null;
}
