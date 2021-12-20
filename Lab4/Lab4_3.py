from mrjob.job import MRJob
from mrjob.step import MRStep
import re

REGEX_ONLY_WORDS = "[\w']+"

class MRDataMining(MRJob):

    def steps(self):
        return [
            MRStep(mapper = self.mapper_get_words, reducer = self.reducer_count_words),
            MRStep(mapper = self.mapper_make_counts_key, reducer = self.reducer_output_words, jobconf = {"mapreduces.job.reduces": 1})
        ]

    def mapper_get_words(self, _, line):
        words = re.findall(REGEX_ONLY_WORDS, line)
        for word in words:
            yield word.lower(), 1
   def reducer_count_words(self, word, values):
        yield word, sum(values)
   def mapper_make_counts_key(self, word, count):
        yield (f'{(999999 - int(count)):06d}', word), word
    def reducer_output_words(self, key, values):
        for value in values:
            yield value, 999999 - int(key[0])

if __name__ == '__main__':
    MRDataMining.run()