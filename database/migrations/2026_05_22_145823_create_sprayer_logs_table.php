<?php

use Illuminate\Database\Migrations\Migration;
use Illuminate\Database\Schema\Blueprint;
use Illuminate\Support\Facades\Schema;

return new class extends Migration
{
    /**
     * Run the migrations.
     */
    public function up(): void
    {
        Schema::create('sprayer_log', function (Blueprint $table) {
            $table->id();
            $table->enum('aksi', ['aktif', 'nonaktif']);
            $table->enum('triggered_by', ['otomatis', 'manual']);
            $table->timestamp('created_at')->useCurrent();
        });
    }

    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        Schema::dropIfExists('sprayer_log');
    }
};
