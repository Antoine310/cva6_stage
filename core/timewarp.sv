// Module protection miss/hit

module timewarp
  import ariane_pkg::*;
#(
    parameter config_pkg::cva6_cfg_t CVA6Cfg = config_pkg::cva6_cfg_empty,
    parameter int HIT_TIME = 100,
    parameter int STALL_COMMIT= 10
) (
    // Subsystem Clock - SUBSYSTEM
    input logic clk_i,
    // Asynchronous reset active low - SUBSYSTEM
    input logic rst_ni,
    // Lecture du cycle en cours dans csr_regfile 
    input logic csr_lecture_cycle,
    // Miss du Dcache 
    input logic dcache_hit_i,
    // Lock commit 
    output logic protect_en_o
);
    logic [$clog2(HIT_TIME+1)-1:0] compteur_hit;
    logic [$clog2(STALL_COMMIT+1)-1:0] compteur_stall;

    logic hit_en ;

    always_comb begin : activation_stall

        protect_en_o = 1'b0; 
        if (hit_en && csr_lecture_cycle) begin 
            protect_en_o = 1'b1; 
            $display("TIMEWARP PROTECTION ENABLED");

        end else if (compteur_stall !=  0) begin
            protect_en_o = 1'b1;
        end 
    end
    
    always_ff @(posedge clk_i or negedge rst_ni) begin
        if (~rst_ni) begin
            hit_en <= 1'b0;
            compteur_hit <= '0;
            compteur_stall <= '0;
        end else begin 

            if (dcache_hit_i) begin
                hit_en <= 1'b1; 
                compteur_hit <= HIT_TIME[$bits(compteur_hit)-1:0];

            end else if (compteur_hit != 0) begin
                compteur_hit <= compteur_hit - 1 ; 
                if (compteur_hit == 1) begin
                    hit_en <= 1'b0; 
                end
            end 

            if (hit_en && csr_lecture_cycle) begin 
                hit_en <= 1'b0; 
                compteur_stall <= STALL_COMMIT[$bits(compteur_stall)-1:0];
            end else if (compteur_stall !=  0) begin
                compteur_stall <= compteur_stall - 1 ; 
            end 

             if (csr_lecture_cycle) begin 
                $display("TIMEWARP PROTECTION");
            end
        end 
    end



endmodule 
