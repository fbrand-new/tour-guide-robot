import yarp

class Module(yarp.RFModule):

    def configure(self, rf):
        self.textportname_i = "/cutString/text:i"
        self.texportname_o = "/cutString/text:o"

        self.textport_i = yarp.BufferedPortBottle()
        self.textport_i.open(self.textportname_i)

        self.textport_o = yarp.Port()
        self.textport_o.open(self.texportname_o)

        return True

    def updateModule(self):

        bot = self.textport_i.read()

        if bot:
            text = bot.get(0)
            text = text.toString()

            if text != "":

                # # Cut out the number in the last position
                # splitted_text = text.split(" ")
                # splitted_text = splitted_text[:-1]
                # text_out  = " ".join(splitted_text)

                # Avoid forwarding quotes
                text_out = text.strip('"')

                bot_out = yarp.Bottle()
                bot_out.addString(text_out)

                self.textport_o.write(bot_out)

        return True

    def interruptModule(self):
        self.textport_i.close()
        self.textport_o.close()
        return True

    def close(self):
        self.textport_i.close()
        self.textport_o.close()
        return True

    def getPeriod(self):
        return 0.1
    

if __name__ == "__main__":

    rf = yarp.ResourceFinder()
    mod = Module()
    try:
        mod.runModule(rf)
    finally:
        mod.interruptModule()
