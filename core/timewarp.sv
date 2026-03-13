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

    logic [6:0] compteur_hit ; 
    logic [3:0] compteur_stall; 

    logic hit_en ;

    always_ff @(posedge clk_i or negedge rst_ni) begin
        if (~rst_ni) begin
            hit_en <= 1'b0;
            compteur_hit <= '0;
            protect_en_o <= 1'b0;
            compteur_stall <= '0;
        end else begin 

            protect_en_o <= 1'b0; 

            if (dcache_hit_i) begin
                hit_en <= 1'b1; 
                compteur_hit <= HIT_TIME ; 
            end else if (compteur_hit != 0) begin
                compteur_hit <= compteur_hit - 1 ; 
            end 

            if (compteur_hit == 0) begin
                hit_en <= 1'b0; 
            end

            if (hit_en && csr_lecture_cycle) begin 
                protect_en_o <= 1'b1; 
                compteur_stall <= STALL_COMMIT; 
                hit_en <= 1'b0; 
            end else if (compteur !=  0) begin
                protect_en_o <= 1'b1;
                compteur_stall <= compteur_stall - 1 ; 
            end 

        end 
    end

endmodule 
